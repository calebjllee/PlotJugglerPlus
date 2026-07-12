#include "dataload_mf4.h"

#include <QApplication>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>

#include <cmath>
#include <limits>
#include <unordered_map>

namespace
{

QString pythonExecutable()
{
  const QByteArray configured = qgetenv("PJ_ASAMMDF_PYTHON");
  if (!configured.isEmpty())
  {
    return QString::fromLocal8Bit(configured);
  }
  return QStringLiteral("python");
}

QString asammdfBridgeScript()
{
  return QStringLiteral(R"PY(
import json
import math
import sys

try:
    import numpy as np
    from asammdf import MDF
except Exception as exc:
    print(json.dumps({
        "type": "error",
        "message": "Python package 'asammdf' is required to load MF4/MDF files: %s" % exc
    }), flush=True)
    sys.exit(2)

filename = sys.argv[1]
chunk_size = 10000

def make_unique(base, used):
    name = base or "unnamed"
    if name not in used:
        used[name] = 1
        return name
    used[name] += 1
    return "%s[%d]" % (name, used[name])

def is_numeric_array(values):
    try:
        dtype = values.dtype
    except Exception:
        return False
    return dtype.kind in "buif"

try:
    mdf = MDF(filename)
except Exception as exc:
    print(json.dumps({
        "type": "error",
        "message": "Failed to open MF4/MDF file: %s" % exc
    }), flush=True)
    sys.exit(3)

used_names = {}
loaded_channels = 0
loaded_points = 0

try:
    channels = mdf.iter_channels(skip_master=True)
except TypeError:
    channels = mdf.iter_channels()

for signal in channels:
    samples = getattr(signal, "samples", None)
    timestamps = getattr(signal, "timestamps", None)
    if samples is None or timestamps is None:
        continue

    samples = np.asarray(samples)
    timestamps = np.asarray(timestamps, dtype=float)

    if samples.ndim != 1 or timestamps.ndim != 1:
        continue
    if len(samples) == 0 or len(samples) != len(timestamps):
        continue
    if not is_numeric_array(samples):
        continue

    samples = samples.astype(float, copy=False)

    raw_name = getattr(signal, "name", "") or "unnamed"
    group_name = ""
    source = getattr(signal, "source", None)
    if source is not None:
        group_name = getattr(source, "name", "") or ""

    if group_name and not raw_name.startswith(group_name + "/"):
        base_name = "%s/%s" % (group_name, raw_name)
    else:
        base_name = raw_name

    series_name = make_unique(base_name.replace("\\", "/"), used_names)
    unit = getattr(signal, "unit", "") or ""

    print(json.dumps({
        "type": "series",
        "name": series_name,
        "unit": unit,
        "count": int(len(samples))
    }), flush=True)

    loaded_channels += 1

    for start in range(0, len(samples), chunk_size):
        end = min(start + chunk_size, len(samples))
        t_chunk = timestamps[start:end]
        v_chunk = samples[start:end]
        mask = np.isfinite(t_chunk) & np.isfinite(v_chunk)
        if not np.any(mask):
            continue

        t_out = t_chunk[mask].astype(float, copy=False).tolist()
        v_out = v_chunk[mask].astype(float, copy=False).tolist()
        loaded_points += len(v_out)

        print(json.dumps({
            "type": "chunk",
            "name": series_name,
            "t": t_out,
            "v": v_out
        }, allow_nan=False), flush=True)

print(json.dumps({
    "type": "done",
    "channels": loaded_channels,
    "points": loaded_points
}), flush=True)
)PY");
}

void appendChunk(const QJsonObject& object, PJ::PlotDataMapRef& destination,
                 std::unordered_map<std::string, PJ::PlotData*>& series_cache)
{
  const QString name_qt = object.value(QStringLiteral("name")).toString();
  if (name_qt.isEmpty())
  {
    return;
  }

  const std::string name = name_qt.toStdString();
  auto cache_it = series_cache.find(name);
  if (cache_it == series_cache.end())
  {
    cache_it = series_cache.emplace(name, &destination.getOrCreateNumeric(name)).first;
  }

  PJ::PlotData* series = cache_it->second;
  const QJsonArray timestamps = object.value(QStringLiteral("t")).toArray();
  const QJsonArray values = object.value(QStringLiteral("v")).toArray();
  const int count = std::min(timestamps.size(), values.size());

  for (int i = 0; i < count; i++)
  {
    const double timestamp = timestamps.at(i).toDouble(std::numeric_limits<double>::quiet_NaN());
    const double value = values.at(i).toDouble(std::numeric_limits<double>::quiet_NaN());
    if (std::isfinite(timestamp) && std::isfinite(value))
    {
      series->pushBack({ timestamp, value });
    }
  }
}

bool processJsonLine(const QByteArray& line, PJ::PlotDataMapRef& destination,
                     std::unordered_map<std::string, PJ::PlotData*>& series_cache,
                     QString& error_message, size_t& loaded_channels, size_t& loaded_points)
{
  QJsonParseError parse_error;
  const QJsonDocument doc = QJsonDocument::fromJson(line, &parse_error);
  if (parse_error.error != QJsonParseError::NoError || !doc.isObject())
  {
    error_message = QStringLiteral("Invalid response from asammdf bridge: %1")
                        .arg(parse_error.errorString());
    return false;
  }

  const QJsonObject object = doc.object();
  const QString type = object.value(QStringLiteral("type")).toString();

  if (type == QStringLiteral("error"))
  {
    error_message = object.value(QStringLiteral("message")).toString();
    return false;
  }
  if (type == QStringLiteral("series"))
  {
    loaded_channels++;
    return true;
  }
  if (type == QStringLiteral("chunk"))
  {
    appendChunk(object, destination, series_cache);
    loaded_points += size_t(object.value(QStringLiteral("v")).toArray().size());
    return true;
  }
  if (type == QStringLiteral("done"))
  {
    return true;
  }

  return true;
}

}  // namespace

const std::vector<const char*>& DataLoadMF4::compatibleFileExtensions() const
{
  static std::vector<const char*> extensions = { "mf4", "mdf" };
  return extensions;
}

bool DataLoadMF4::readDataFromFile(PJ::FileLoadInfo* fileload_info,
                                   PJ::PlotDataMapRef& destination)
{
  QProcess process;
  process.setProgram(pythonExecutable());
  process.setArguments({ QStringLiteral("-u"), QStringLiteral("-c"), asammdfBridgeScript(),
                         fileload_info->filename });
  process.setProcessChannelMode(QProcess::SeparateChannels);

  QProgressDialog progress_dialog(QStringLiteral("Loading MF4/MDF file..."),
                                  QStringLiteral("Cancel"), 0, 0, nullptr);
  progress_dialog.setWindowTitle(QStringLiteral("Loading MF4/MDF"));
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.show();

  process.start();
  if (!process.waitForStarted())
  {
    QMessageBox::warning(nullptr, QStringLiteral("MF4 loader"),
                         QStringLiteral("Failed to start Python executable '%1'.\n\n"
                                        "Set PJ_ASAMMDF_PYTHON to the Python executable "
                                        "that has the 'asammdf' package installed.")
                             .arg(pythonExecutable()));
    return false;
  }

  QByteArray pending_stdout;
  QString error_message;
  std::unordered_map<std::string, PJ::PlotData*> series_cache;
  size_t loaded_channels = 0;
  size_t loaded_points = 0;

  auto consume_stdout = [&]() -> bool {
    pending_stdout += process.readAllStandardOutput();
    while (true)
    {
      const int newline_index = pending_stdout.indexOf('\n');
      if (newline_index < 0)
      {
        break;
      }
      const QByteArray line = pending_stdout.left(newline_index).trimmed();
      pending_stdout.remove(0, newline_index + 1);
      if (!line.isEmpty() &&
          !processJsonLine(line, destination, series_cache, error_message, loaded_channels,
                           loaded_points))
      {
        return false;
      }
    }
    return true;
  };

  while (process.state() != QProcess::NotRunning)
  {
    process.waitForReadyRead(100);
    if (!consume_stdout())
    {
      process.kill();
      process.waitForFinished(1000);
      QMessageBox::warning(nullptr, QStringLiteral("MF4 loader"), error_message);
      return false;
    }

    progress_dialog.setLabelText(
        QStringLiteral("Loading MF4/MDF file...\n%1 channels, %2 points")
            .arg(loaded_channels)
            .arg(loaded_points));
    QApplication::processEvents();

    if (progress_dialog.wasCanceled())
    {
      process.kill();
      process.waitForFinished(1000);
      return false;
    }
  }

  if (!consume_stdout())
  {
    QMessageBox::warning(nullptr, QStringLiteral("MF4 loader"), error_message);
    return false;
  }
  if (!pending_stdout.trimmed().isEmpty())
  {
    if (!processJsonLine(pending_stdout.trimmed(), destination, series_cache, error_message,
                         loaded_channels, loaded_points))
    {
      QMessageBox::warning(nullptr, QStringLiteral("MF4 loader"), error_message);
      return false;
    }
  }

  if (!error_message.isEmpty())
  {
    QMessageBox::warning(nullptr, QStringLiteral("MF4 loader"), error_message);
    return false;
  }

  const QByteArray stderr_output = process.readAllStandardError().trimmed();
  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
  {
    const QString details = QString::fromLocal8Bit(stderr_output);
    QMessageBox::warning(nullptr, QStringLiteral("MF4 loader"),
                         details.isEmpty() ? QStringLiteral("MF4 import failed.")
                                           : details);
    return false;
  }

  if (loaded_points == 0)
  {
    QMessageBox::warning(nullptr, QStringLiteral("MF4 loader"),
                         QStringLiteral("No numeric MF4/MDF channels were loaded."));
    return false;
  }

  return true;
}

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PLOTMAGNIFIER_H
#define PLOTMAGNIFIER_H

#include <QTimer>
#include "qwt_plot_magnifier.h"
#include "qwt_plot.h"
#include <QEvent>

class PlotMagnifier : public QwtPlotMagnifier
{
  Q_OBJECT

public:
  explicit PlotMagnifier(QWidget* canvas);
  virtual ~PlotMagnifier() override;

  void setAxisLimits(int axis, double lower, double upper);
  virtual void widgetWheelEvent(QWheelEvent* event) override;

  enum AxisMode
  {
    X_AXIS,
    Y_AXIS,
    BOTH_AXES
  };

  virtual void rescale(double factor) override
  {
    rescale(factor, _default_mode);
  }

  void setDefaultMode(AxisMode mode)
  {
    _default_mode = mode;
  }

  void rescale(double factor, AxisMode axis);

protected:
  virtual void widgetMousePressEvent(QMouseEvent* event) override;

  double _lower_bounds[QwtPlot::axisCnt];
  double _upper_bounds[QwtPlot::axisCnt];

  QPointF _mouse_position;

signals:
  void rescaled(QRectF new_size);

private:
  QPointF invTransform(QPoint pos);
  QPointF invTransform(QPoint pos, int y_axis_id);
  QTimer _future_emit;
  AxisMode _default_mode;
  double _mouse_y_left = 0.0;
  double _mouse_y_right = 0.0;
};

#endif  // PLOTMAGNIFIER_H

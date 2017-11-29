#pragma once

#include <QtWidgets/QWidget>
#include "ui_DXGICapture.h"
#include "DXGIDevices.h"

class DXGICapture : public QWidget
{
	Q_OBJECT

public:
	DXGICapture(QWidget *parent = Q_NULLPTR);
	HRESULT CaptureFrame();
private:
	Ui::DXGICaptureClass ui;
	//DXGIDevices m_device;
};

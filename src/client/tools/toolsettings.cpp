/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2006-2014 Calle Laakkonen

   Drawpile is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Drawpile is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Drawpile.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QDebug>
#include <QSettings>
#include <QTimer>

#include "toolsettings.h"
#include "docks/layerlistdock.h"
#include "widgets/brushpreview.h"
#include "widgets/colorbutton.h"
#include "widgets/palettewidget.h"
using widgets::BrushPreview; // qt designer doesn't know about namespaces (TODO works in qt5?)
using widgets::ColorButton;
#include "ui_pensettings.h"
#include "ui_brushsettings.h"
#include "ui_erasersettings.h"
#include "ui_simplesettings.h"
#include "ui_textsettings.h"
#include "ui_selectsettings.h"
#include "ui_lasersettings.h"

#include "scene/annotationitem.h"
#include "net/client.h"

#include "utils/palette.h"

#include "core/annotation.h"
#include "core/rasterop.h" // for blend modes

namespace tools {

QWidget *ToolSettings::createUi(QWidget *parent)
{
	Q_ASSERT(_widget==0);
	_widget = createUiWidget(parent);
	restoreSettings();
	return _widget;
}

void ToolSettings::saveSettings()
{
	Q_ASSERT(_widget);
	QSettings cfg;
	cfg.beginGroup("tools");
	cfg.beginGroup(_name);
	saveToolSettings(cfg);
}

void ToolSettings::restoreSettings()
{
	Q_ASSERT(_widget);
	QSettings cfg;
	cfg.beginGroup("tools");
	cfg.beginGroup(_name);
	restoreToolSettings(cfg);
}

PenSettings::PenSettings(QString name, QString title)
	: ToolSettings(name, title), _ui(0)
{
}

PenSettings::~PenSettings()
{
	if(_ui) {
		saveSettings();
		delete _ui;
	}
}

QWidget *PenSettings::createUiWidget(QWidget *parent)
{
	QWidget *widget = new QWidget(parent);
	_ui = new Ui_PenSettings;
	_ui->setupUi(widget);

	// Populate blend mode combobox
	// Blend mode 0 is reserved for the eraser
	for(int b=1;b<paintcore::BLEND_MODES;++b) {
		_ui->blendmode->addItem(QApplication::tr(paintcore::BLEND_MODE[b]));
	}

	// Connect size change signal
	parent->connect(_ui->brushsize, SIGNAL(valueChanged(int)), parent, SIGNAL(sizeChanged(int)));
	return widget;
}

void PenSettings::restoreToolSettings(QSettings &cfg)
{
	_ui->incremental->setChecked(cfg.value("incremental", true).toBool());
	_ui->preview->setIncremental(_ui->incremental->isChecked());

	_ui->brushsize->setValue(cfg.value("size", 0).toInt());
	_ui->preview->setSize(_ui->brushsize->value());

	_ui->brushopacity->setValue(cfg.value("opacity", 100).toInt());
	_ui->preview->setOpacity(_ui->brushopacity->value());

	_ui->preview->setHardness(100);

	_ui->brushspacing->setValue(cfg.value("spacing", 15).toInt());
	_ui->preview->setSpacing(_ui->brushspacing->value());

	_ui->pressuresize->setChecked(cfg.value("pressuresize",false).toBool());
	_ui->preview->setSizePressure(_ui->pressuresize->isChecked());

	_ui->pressureopacity->setChecked(cfg.value("pressureopacity",false).toBool());
	_ui->preview->setOpacityPressure(_ui->pressureopacity->isChecked());

	_ui->pressurecolor->setChecked(cfg.value("pressurecolor",false).toBool());
	_ui->preview->setColorPressure(_ui->pressurecolor->isChecked());

	_ui->preview->setSubpixel(false);
}

void PenSettings::saveToolSettings(QSettings &cfg)
{
	cfg.setValue("blendmode", _ui->blendmode->currentIndex());
	cfg.setValue("incremental", _ui->incremental->isChecked());
	cfg.setValue("size", _ui->brushsize->value());
	cfg.setValue("opacity", _ui->brushopacity->value());
	cfg.setValue("spacing", _ui->brushspacing->value());
	cfg.setValue("pressuresize", _ui->pressuresize->isChecked());
	cfg.setValue("pressureopacity", _ui->pressureopacity->isChecked());
	cfg.setValue("pressurecolor", _ui->pressurecolor->isChecked());
}

void PenSettings::setForeground(const QColor& color)
{
	_ui->preview->setColor1(color);
}

void PenSettings::setBackground(const QColor& color)
{
	_ui->preview->setColor2(color);
}

void PenSettings::quickAdjust1(float adjustment)
{
	int adj = qRound(adjustment);
	if(adj!=0)
		_ui->brushsize->setValue(_ui->brushsize->value() + adj);
}

const paintcore::Brush& PenSettings::getBrush(bool swapcolors) const
{
	return _ui->preview->brush(swapcolors);
}

int PenSettings::getSize() const
{
	return _ui->brushsize->value();
}

EraserSettings::EraserSettings(QString name, QString title)
	: ToolSettings(name,title), _ui(0)
{
}

EraserSettings::~EraserSettings()
{
	if(_ui) {
		saveSettings();
		delete _ui;
	}
}

QWidget *EraserSettings::createUiWidget(QWidget *parent)
{
	QWidget *widget = new QWidget(parent);
	_ui = new Ui_EraserSettings();
	_ui->setupUi(widget);

	_ui->preview->setBlendingMode(-1); // eraser is normally not visible

	parent->connect(_ui->hardedge, &QToolButton::toggled, [this](bool hard) { _ui->brushhardness->setEnabled(!hard); });
	parent->connect(_ui->brushsize, SIGNAL(valueChanged(int)), parent, SIGNAL(sizeChanged(int)));

	return widget;
}

void EraserSettings::saveToolSettings(QSettings &cfg)
{
	cfg.setValue("size", _ui->brushsize->value());
	cfg.setValue("opacity", _ui->brushopacity->value());
	cfg.setValue("hardness", _ui->brushhardness->value());
	cfg.setValue("spacing", _ui->brushspacing->value());
	cfg.setValue("pressuresize", _ui->pressuresize->isChecked());
	cfg.setValue("pressureopacity", _ui->pressureopacity->isChecked());
	cfg.setValue("pressurehardness", _ui->pressurehardness->isChecked());
	cfg.setValue("hardedge", _ui->hardedge->isChecked());
	cfg.setValue("incremental", _ui->incremental->isChecked());
}

void EraserSettings::restoreToolSettings(QSettings &cfg)
{
	_ui->brushsize->setValue(cfg.value("size", 0).toInt());
	_ui->preview->setSize(_ui->brushsize->value());

	_ui->brushopacity->setValue(cfg.value("opacity", 100).toInt());
	_ui->preview->setOpacity(_ui->brushopacity->value());

	_ui->brushhardness->setValue(cfg.value("hardness", 50).toInt());
	_ui->preview->setHardness(_ui->brushhardness->value());

	_ui->brushspacing->setValue(cfg.value("spacing", 15).toInt());
	_ui->preview->setSpacing(_ui->brushspacing->value());

	_ui->pressuresize->setChecked(cfg.value("pressuresize",false).toBool());
	_ui->preview->setSizePressure(_ui->pressuresize->isChecked());

	_ui->pressureopacity->setChecked(cfg.value("pressureopacity",false).toBool());
	_ui->preview->setOpacityPressure(_ui->pressureopacity->isChecked());

	_ui->pressurehardness->setChecked(cfg.value("pressurehardness",false).toBool());
	_ui->preview->setHardnessPressure(_ui->pressurehardness->isChecked());

	_ui->hardedge->setChecked(cfg.value("hardedge", false).toBool());

	_ui->incremental->setChecked(cfg.value("incremental", true).toBool());
	_ui->preview->setIncremental(_ui->incremental->isChecked());
}

void EraserSettings::setForeground(const QColor& color)
{
	// Eraser has no foreground color
}

void EraserSettings::setBackground(const QColor& color)
{
	// This is used just for the preview background color
	_ui->preview->setColor2(color);
}

void EraserSettings::quickAdjust1(float adjustment)
{
	int adj = qRound(adjustment);
	if(adj!=0)
		_ui->brushsize->setValue(_ui->brushsize->value() + adj);
}

const paintcore::Brush& EraserSettings::getBrush(bool swapcolors) const
{
	return _ui->preview->brush(swapcolors);
}

int EraserSettings::getSize() const
{
	return _ui->brushsize->value();
}

BrushSettings::BrushSettings(QString name, QString title)
	: ToolSettings(name,title), _ui(0)
{
}

BrushSettings::~BrushSettings()
{
	if(_ui) {
		saveSettings();
		delete _ui;
	}
}

QWidget *BrushSettings::createUiWidget(QWidget *parent)
{
	QWidget *widget = new QWidget(parent);
	_ui = new Ui_BrushSettings;
	_ui->setupUi(widget);

	// Populate blend mode combobox
	for(int b=1;b<paintcore::BLEND_MODES;++b) {
		_ui->blendmode->addItem(QApplication::tr(paintcore::BLEND_MODE[b]));
	}

	// Connect size change signal
	parent->connect(_ui->brushsize, SIGNAL(valueChanged(int)), parent, SIGNAL(sizeChanged(int)));

	return widget;
}

void BrushSettings::saveToolSettings(QSettings &cfg)
{
	cfg.setValue("blendmode", _ui->blendmode->currentIndex());
	cfg.setValue("incremental", _ui->incremental->isChecked());
	cfg.setValue("size", _ui->brushsize->value());
	cfg.setValue("opacity", _ui->brushopacity->value());
	cfg.setValue("hardness", _ui->brushhardness->value());
	cfg.setValue("spacing", _ui->brushspacing->value());
	cfg.setValue("pressuresize", _ui->pressuresize->isChecked());
	cfg.setValue("pressureopacity", _ui->pressureopacity->isChecked());
	cfg.setValue("pressurehardness", _ui->pressurehardness->isChecked());
	cfg.setValue("pressurecolor", _ui->pressurecolor->isChecked());
}

void BrushSettings::restoreToolSettings(QSettings &cfg)
{
	_ui->blendmode->setCurrentIndex(cfg.value("blendmode", 0).toInt());

	_ui->incremental->setChecked(cfg.value("incremental", true).toBool());
	_ui->preview->setIncremental(_ui->incremental->isChecked());

	_ui->brushsize->setValue(cfg.value("size", 0).toInt());
	_ui->preview->setSize(_ui->brushsize->value());

	_ui->brushopacity->setValue(cfg.value("opacity", 100).toInt());
	_ui->preview->setOpacity(_ui->brushopacity->value());

	_ui->brushhardness->setValue(cfg.value("hardness", 50).toInt());
	_ui->preview->setHardness(_ui->brushhardness->value());

	_ui->brushspacing->setValue(cfg.value("spacing", 15).toInt());
	_ui->preview->setSpacing(_ui->brushspacing->value());

	_ui->pressuresize->setChecked(cfg.value("pressuresize",false).toBool());
	_ui->preview->setSizePressure(_ui->pressuresize->isChecked());

	_ui->pressureopacity->setChecked(cfg.value("pressureopacity",false).toBool());
	_ui->preview->setOpacityPressure(_ui->pressureopacity->isChecked());

	_ui->pressurehardness->setChecked(cfg.value("pressurehardness",false).toBool());
	_ui->preview->setHardnessPressure(_ui->pressurehardness->isChecked());

	_ui->pressurecolor->setChecked(cfg.value("pressurecolor",false).toBool());
	_ui->preview->setColorPressure(_ui->pressurecolor->isChecked());

	_ui->preview->setSubpixel(true);
}

void BrushSettings::setForeground(const QColor& color)
{
	_ui->preview->setColor1(color);
}

void BrushSettings::setBackground(const QColor& color)
{
	_ui->preview->setColor2(color);
}

void BrushSettings::quickAdjust1(float adjustment)
{
	int adj = qRound(adjustment);
	if(adj!=0)
		_ui->brushsize->setValue(_ui->brushsize->value() + adj);
}

const paintcore::Brush& BrushSettings::getBrush(bool swapcolors) const
{
	return _ui->preview->brush(swapcolors);
}

int BrushSettings::getSize() const
{
	return _ui->brushsize->value();
}

void BrushlessSettings::setForeground(const QColor& color)
{
	_dummybrush.setColor(color);
}

void BrushlessSettings::setBackground(const QColor& color)
{
	_dummybrush.setColor2(color);
}

const paintcore::Brush& BrushlessSettings::getBrush(bool swapcolors) const
{
	Q_UNUSED(swapcolors);
	return _dummybrush;
}

LaserPointerSettings::LaserPointerSettings(const QString &name, const QString &title)
	: QObject(), BrushlessSettings(name, title), _ui(0)
{
}

LaserPointerSettings::~LaserPointerSettings()
{
	if(_ui) {
		saveSettings();
		delete _ui;
	}
}

QWidget *LaserPointerSettings::createUiWidget(QWidget *parent)
{
	QWidget *widget = new QWidget(parent);
	_ui = new Ui_LaserSettings;
	_ui->setupUi(widget);

	connect(_ui->trackpointer, SIGNAL(clicked(bool)), this, SIGNAL(pointerTrackingToggled(bool)));

	return widget;
}

void LaserPointerSettings::saveToolSettings(QSettings &cfg)
{
	cfg.setValue("tracking", _ui->trackpointer->isChecked());
	cfg.setValue("persistence", _ui->persistence->value());

	int color=0;

	if(_ui->color1->isChecked())
		color=1;
	else if(_ui->color2->isChecked())
		color=2;
	else if(_ui->color3->isChecked())
		color=3;
	cfg.setValue("color", color);
}

void LaserPointerSettings::restoreToolSettings(QSettings &cfg)
{
	_ui->trackpointer->setChecked(cfg.value("tracking", true).toBool());
	_ui->persistence->setValue(cfg.value("persistence", 1).toInt());

	switch(cfg.value("color", 0).toInt()) {
	case 1: _ui->color1->setChecked(true); break;
	case 2: _ui->color2->setChecked(true); break;
	case 3: _ui->color3->setChecked(true); break;
	default: _ui->color0->setChecked(true); break;
	}
}

bool LaserPointerSettings::pointerTracking() const
{
	return _ui->trackpointer->isChecked();
}

int LaserPointerSettings::trailPersistence() const
{
	return _ui->persistence->value();
}

void LaserPointerSettings::setForeground(const QColor &color)
{
	_ui->color0->setColor(color);
}

void LaserPointerSettings::quickAdjust1(float adjustment)
{
	int adj = qRound(adjustment);
	if(adj!=0)
		_ui->persistence->setValue(_ui->persistence->value() + adj);
}

const paintcore::Brush& LaserPointerSettings::getBrush(bool swapcolors) const
{
	QColor c;
	if(swapcolors)
		c = _dummybrush.color2();
	else {
		if(_ui->color0->isChecked())
			c = _ui->color0->color();
		else if(_ui->color1->isChecked())
			c = _ui->color1->color();
		else if(_ui->color2->isChecked())
			c = _ui->color2->color();
		else if(_ui->color3->isChecked())
			c = _ui->color3->color();
	}
	const_cast<paintcore::Brush&>(_dummybrush).setColor(c);
	return _dummybrush;
}

SimpleSettings::SimpleSettings(QString name, QString title, Type type, bool sp)
	: ToolSettings(name,title), _ui(0), _type(type), _subpixel(sp)
{
}

SimpleSettings::~SimpleSettings()
{
	if(_ui) {
		saveSettings();
		delete _ui;
	}
}

QWidget *SimpleSettings::createUiWidget(QWidget *parent)
{
	QWidget *widget = new QWidget(parent);
	_ui = new Ui_SimpleSettings;
	_ui->setupUi(widget);

	// Populate blend mode combobox
	for(int b=1;b<paintcore::BLEND_MODES;++b) {
		_ui->blendmode->addItem(QApplication::tr(paintcore::BLEND_MODE[b]));
	}

	// Connect size change signal
	parent->connect(_ui->brushsize, SIGNAL(valueChanged(int)), parent, SIGNAL(sizeChanged(int)));
	parent->connect(_ui->hardedge, &QToolButton::toggled, [this](bool hard) { _ui->brushhardness->setEnabled(!hard); });

	// Set proper preview shape
	if(_type==Line)
		_ui->preview->setPreviewShape(BrushPreview::Line);
	else if(_type==Rectangle)
		_ui->preview->setPreviewShape(BrushPreview::Rectangle);
	else if(_type==Ellipse)
		_ui->preview->setPreviewShape(BrushPreview::Ellipse);

	_ui->preview->setSubpixel(_subpixel);

	return widget;
}

void SimpleSettings::saveToolSettings(QSettings &cfg)
{
	cfg.setValue("blendmode", _ui->blendmode->currentIndex());
	cfg.setValue("incremental", _ui->incremental->isChecked());
	cfg.setValue("size", _ui->brushsize->value());
	cfg.setValue("opacity", _ui->brushopacity->value());
	cfg.setValue("hardness", _ui->brushhardness->value());
	cfg.setValue("spacing", _ui->brushspacing->value());
	cfg.setValue("hardedge", _ui->hardedge->isChecked());
}

void SimpleSettings::restoreToolSettings(QSettings &cfg)
{
	_ui->blendmode->setCurrentIndex(cfg.value("blendmode", 0).toInt());

	_ui->incremental->setChecked(cfg.value("incremental", true).toBool());
	_ui->preview->setIncremental(_ui->incremental->isChecked());

	_ui->brushsize->setValue(cfg.value("size", 0).toInt());
	_ui->preview->setSize(_ui->brushsize->value());

	_ui->brushopacity->setValue(cfg.value("opacity", 100).toInt());
	_ui->preview->setOpacity(_ui->brushopacity->value());

	_ui->brushhardness->setValue(cfg.value("hardness", 50).toInt());
	_ui->preview->setHardness(_ui->brushhardness->value());

	_ui->brushspacing->setValue(cfg.value("spacing", 15).toInt());
	_ui->preview->setSpacing(_ui->brushspacing->value());

	_ui->hardedge->setChecked(cfg.value("hardedge", false).toBool());

	if(!_subpixel) {
		// If subpixel accuracy wasn't enabled, don't offer a chance to
		// enable it.
		_ui->hardedge->hide();
		_ui->brushopts->addSpacing(_ui->hardedge->width());
	}
}

void SimpleSettings::setForeground(const QColor& color)
{
	_ui->preview->setColor1(color);
}

void SimpleSettings::setBackground(const QColor& color)
{
	_ui->preview->setColor2(color);
}

void SimpleSettings::quickAdjust1(float adjustment)
{
	int adj = qRound(adjustment);
	if(adj!=0)
		_ui->brushsize->setValue(_ui->brushsize->value() + adj);
}

const paintcore::Brush& SimpleSettings::getBrush(bool swapcolors) const
{
	return _ui->preview->brush(swapcolors);
}

int SimpleSettings::getSize() const
{
	return _ui->brushsize->value();
}

ColorPickerSettings::ColorPickerSettings(const QString &name, const QString &title)
	:  QObject(), BrushlessSettings(name, title), _palette(new Palette("Color picker")), _layerpick(0)
{
}

ColorPickerSettings::~ColorPickerSettings()
{
	if(getUi())
		saveSettings();
}

QWidget *ColorPickerSettings::createUiWidget(QWidget *parent)
{
	QWidget *widget = new QWidget(parent);
	QVBoxLayout *layout = new QVBoxLayout(widget);
	widget->setLayout(layout);

	_layerpick = new QCheckBox(widget->tr("Pick from current layer only"), widget);
	layout->addWidget(_layerpick);

	_palettewidget = new widgets::PaletteWidget(widget);
	_palettewidget->setPalette(_palette);
	_palettewidget->setSwatchSize(32, 24);
	_palettewidget->setSpacing(3);
	layout->addWidget(_palettewidget);

	connect(_palettewidget, SIGNAL(colorSelected(QColor)), this, SIGNAL(colorSelected(QColor)));

	return widget;
}

void ColorPickerSettings::saveToolSettings(QSettings &cfg)
{
	cfg.setValue("layerpick", _layerpick->isChecked());
}

void ColorPickerSettings::restoreToolSettings(QSettings &cfg)
{
	_layerpick->setChecked(cfg.value("layerpick", false).toBool());
}

bool ColorPickerSettings::pickFromLayer() const
{
	return _layerpick->isChecked();
}

void ColorPickerSettings::addColor(const QColor &color)
{
	if(_palette->count() && _palette->color(0) == color)
		return;

	_palette->insertColor(0, color);

	if(_palette->count() > 80)
		_palette->removeColor(_palette->count()-1);

	_palettewidget->update();
}

AnnotationSettings::AnnotationSettings(QString name, QString title)
	: QObject(), BrushlessSettings(name, title), _ui(0), _noupdate(false)
{
}

AnnotationSettings::~AnnotationSettings()
{
	delete _ui;
}

QWidget *AnnotationSettings::createUiWidget(QWidget *parent)
{
	QWidget *widget = new QWidget(parent);
	_ui = new Ui_TextSettings;
	_ui->setupUi(widget);
	widget->setEnabled(false);

	_updatetimer = new QTimer(this);
	_updatetimer->setInterval(500);
	_updatetimer->setSingleShot(true);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
	// Set editor placeholder
	_ui->content->setPlaceholderText(tr("Annotation content"));
#endif

	// Editor events
	connect(_ui->content, SIGNAL(textChanged()), this, SLOT(applyChanges()));
	connect(_ui->content, SIGNAL(cursorPositionChanged()), this, SLOT(updateStyleButtons()));

	connect(_ui->btnBackground, SIGNAL(colorChanged(const QColor&)),
			this, SLOT(applyChanges()));
	connect(_ui->btnRemove, SIGNAL(clicked()), this, SLOT(removeAnnotation()));
	connect(_ui->btnBake, SIGNAL(clicked()), this, SLOT(bake()));

	// Intra-editor connections that couldn't be made in the UI designer
	connect(_ui->left, SIGNAL(clicked()), this, SLOT(changeAlignment()));
	connect(_ui->center, SIGNAL(clicked()), this, SLOT(changeAlignment()));
	connect(_ui->justify, SIGNAL(clicked()), this, SLOT(changeAlignment()));
	connect(_ui->right, SIGNAL(clicked()), this, SLOT(changeAlignment()));
	connect(_ui->bold, SIGNAL(toggled(bool)), this, SLOT(toggleBold(bool)));
	connect(_ui->strikethrough, SIGNAL(toggled(bool)), this, SLOT(toggleStrikethrough(bool)));

	connect(_updatetimer, SIGNAL(timeout()), this, SLOT(saveChanges()));

	return widget;
}

void AnnotationSettings::updateStyleButtons()
{
	QTextBlockFormat bf = _ui->content->textCursor().blockFormat();
	switch(bf.alignment()) {
	case Qt::AlignLeft: _ui->left->setChecked(true); break;
	case Qt::AlignCenter: _ui->center->setChecked(true); break;
	case Qt::AlignJustify: _ui->justify->setChecked(true); break;
	case Qt::AlignRight: _ui->right->setChecked(true); break;
	default: break;
	}
	QTextCharFormat cf = _ui->content->textCursor().charFormat();
	_ui->btnTextColor->setColor(cf.foreground().color());

	_ui->size->blockSignals(true);
	if(cf.fontPointSize() < 1)
		_ui->size->setValue(12);
	else
		_ui->size->setValue(cf.fontPointSize());
	_ui->size->blockSignals(false);

	_ui->font->blockSignals(true);
	_ui->font->setCurrentFont(cf.font());
	_ui->font->blockSignals(false);

	_ui->italic->setChecked(cf.fontItalic());
	_ui->bold->setChecked(cf.fontWeight() > QFont::Normal);
	_ui->underline->setChecked(cf.fontUnderline());
	_ui->strikethrough->setChecked(cf.font().strikeOut());
}

void AnnotationSettings::toggleBold(bool bold)
{
	_ui->content->setFontWeight(bold ? QFont::Bold : QFont::Normal);
}

void AnnotationSettings::toggleStrikethrough(bool strike)
{
	QFont font = _ui->content->currentFont();
	font.setStrikeOut(strike);
	_ui->content->setCurrentFont(font);
}

void AnnotationSettings::changeAlignment()
{
	Qt::Alignment a = Qt::AlignLeft;
	if(_ui->left->isChecked())
		a = Qt::AlignLeft;
	else if(_ui->center->isChecked())
		a = Qt::AlignCenter;
	else if(_ui->justify->isChecked())
		a = Qt::AlignJustify;
	else if(_ui->right->isChecked())
		a = Qt::AlignRight;

	_ui->content->setAlignment(a);
}

int AnnotationSettings::selected() const
{
	if(_selection.isNull())
		return 0;
	return _selection->id();
}

void AnnotationSettings::unselect(int id)
{
	if(selected() == id)
		setSelection(0);
}

void AnnotationSettings::setSelection(drawingboard::AnnotationItem *item)
{
	_noupdate = true;
	getUi()->setEnabled(item!=0);

	if(_selection)
		_selection->setHighlight(false);

	_selection = item;
	if(item) {
		item->setHighlight(true);
		const paintcore::Annotation *a = item->getAnnotation();
		Q_ASSERT(a);
		_ui->content->setHtml(a->text());
		_ui->btnBackground->setColor(a->backgroundColor());
	}
	_noupdate = false;
}

void AnnotationSettings::applyChanges()
{
	if(_noupdate)
		return;
	Q_ASSERT(selected());
	_updatetimer->start();
}

void AnnotationSettings::saveChanges()
{
	Q_ASSERT(_client);

	if(selected())
		_client->sendAnnotationEdit(
			selected(),
			_ui->btnBackground->color(),
			_ui->content->toHtml()
		);
}

void AnnotationSettings::removeAnnotation()
{
	Q_ASSERT(selected());
	Q_ASSERT(_client);
	_client->sendUndopoint();
	_client->sendAnnotationDelete(selected());
	setSelection(0); /* not strictly necessary, but makes the UI seem more responsive */
}

void AnnotationSettings::bake()
{
	Q_ASSERT(selected());
	Q_ASSERT(_layerlist);
	Q_ASSERT(_client);

	const paintcore::Annotation *a = _selection->getAnnotation();
	Q_ASSERT(a);

	QImage img = a->toImage();

	int layer = _layerlist->currentLayer();
	_client->sendUndopoint();
	_client->sendImage(layer, a->rect().x(), a->rect().y(), img, true);
	_client->sendAnnotationDelete(selected());
	setSelection(0); /* not strictly necessary, but makes the UI seem more responsive */
}

SelectionSettings::SelectionSettings(const QString &name, const QString &title)
	: BrushlessSettings(name,title), _ui(0)
{
}

SelectionSettings::~SelectionSettings()
{
	delete _ui;
}

QWidget *SelectionSettings::createUiWidget(QWidget *parent)
{
	QWidget *uiwidget = new QWidget(parent);
	_ui = new Ui_SelectionSettings;
	_ui->setupUi(uiwidget);

	return uiwidget;
}

}

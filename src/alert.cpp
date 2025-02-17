/*
	SPDX-FileCopyrightText: 2010-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "alert.h"

#include "action_manager.h"

#include <QAction>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QTimeLine>
#include <QToolButton>

//-----------------------------------------------------------------------------

Alert::Alert(QWidget* parent)
	: QWidget(parent)
{
	init();
}

//-----------------------------------------------------------------------------

Alert::Alert(Icon icon, const QString& text, const QStringList& details, bool expandable, QWidget* parent)
	: QWidget(parent)
{
	init();

	setExpandable(expandable);
	setIcon(icon);
	setText(text, details);
}

//-----------------------------------------------------------------------------

void Alert::fadeIn()
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	connect(m_fade_timer, &QTimeLine::finished, this, &Alert::fadeInFinished);
	m_fade_timer->start();
}

//-----------------------------------------------------------------------------

void Alert::setExpandable(bool expandable)
{
	m_always_expanded = !expandable;
	if (m_always_expanded) {
		m_text->setText(m_long_text);
		m_expander->hide();
	} else if (m_short_text != m_long_text) {
		m_expanded = true;
		expanderToggled();
	}
}

//-----------------------------------------------------------------------------

void Alert::setIcon(Icon icon)
{
	QStyle::StandardPixmap pixmap = QStyle::SP_CustomBase;
	switch (icon) {
	case Critical:
		pixmap = QStyle::SP_MessageBoxCritical;
		break;
	case Information:
		pixmap = QStyle::SP_MessageBoxInformation;
		break;
	case Question:
		pixmap = QStyle::SP_MessageBoxQuestion;
		break;
	case Warning:
		pixmap = QStyle::SP_MessageBoxWarning;
		break;
	default:
		break;
	}
	const int size = style()->pixelMetric(QStyle::PM_LargeIconSize);
	setIcon(style()->standardIcon(pixmap).pixmap(size,size));
}

//-----------------------------------------------------------------------------

void Alert::setIcon(const QPixmap& pixmap)
{
	if (!pixmap.isNull()) {
		m_pixmap->setPixmap(pixmap);
	} else {
		m_pixmap->clear();
	}
}

//-----------------------------------------------------------------------------

void Alert::setText(const QString& text, const QStringList& details)
{
	m_short_text = "<p>" + text + "</p>";
	m_long_text = m_short_text;
	if (!details.isEmpty()) {
		m_long_text += "<p><small>" + details.join("<br>") + "</small></p>";
	}

	m_expander->setVisible(!details.isEmpty() && !m_always_expanded);
	m_expanded = true;
	expanderToggled();
}

//-----------------------------------------------------------------------------

bool Alert::eventFilter(QObject* watched, QEvent* event)
{
	if ((watched == parentWidget()) && (event->type() == QEvent::Resize)) {
		m_under_mouse = rect().contains(mapFromGlobal(QCursor::pos()));
		update();
	}
	return QWidget::eventFilter(watched, event);
}

//-----------------------------------------------------------------------------

void Alert::enterEvent(QEnterEvent* event)
{
	m_under_mouse = true;
	update();
	QWidget::enterEvent(event);
}

//-----------------------------------------------------------------------------

void Alert::leaveEvent(QEvent* event)
{
	m_under_mouse = false;
	update();
	QWidget::leaveEvent(event);
}

//-----------------------------------------------------------------------------

void Alert::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setOpacity(m_under_mouse ? 1.0 : 0.8);
	painter.setPen(QPen(palette().color(foregroundRole()), 1.5));
	painter.setBrush(palette().color(backgroundRole()));
	painter.drawRoundedRect(QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5), 7, 7);
}

//-----------------------------------------------------------------------------

void Alert::expanderToggled()
{
	m_expanded = !m_expanded;
	if (m_expanded || m_always_expanded) {
		m_expander->setArrowType(Qt::UpArrow);
		m_expander->setToolTip(tr("Collapse"));
		m_text->setText(m_long_text);
	} else {
		m_expander->setArrowType(Qt::RightArrow);
		m_expander->setToolTip(tr("Expand"));
		m_text->setText(m_short_text);
	}
}

//-----------------------------------------------------------------------------

void Alert::fadeInFinished()
{
	m_fade_effect->setEnabled(false);
	setAttribute(Qt::WA_TransparentForMouseEvents, false);
	disconnect(m_fade_timer, &QTimeLine::finished, this, &Alert::fadeInFinished);
}

//-----------------------------------------------------------------------------

void Alert::fadeOut()
{
	m_fade_effect->setEnabled(true);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	m_fade_timer->setDirection(QTimeLine::Backward);
	connect(m_fade_timer, &QTimeLine::finished, this, &Alert::deleteLater);
	m_fade_timer->start();
}

//-----------------------------------------------------------------------------

void Alert::init()
{
	m_expanded = true;
	m_always_expanded = false;
	m_under_mouse = false;

	setAttribute(Qt::WA_TranslucentBackground);
	setStyleSheet("QLabel, QToolButton { color: white } Alert { color: white; background-color: black }");

	if (parent()) {
		parent()->installEventFilter(this);
	}

	m_expander = new QToolButton(this);
	m_expander->setAutoRaise(true);
	m_expander->setIconSize(QSize(16,16));
	m_expander->hide();
	connect(m_expander, &QToolButton::clicked, this, &Alert::expanderToggled);

	m_pixmap = new QLabel(this);
	m_pixmap->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

	m_text = new QLabel(this);
	m_text->setTextInteractionFlags(Qt::TextSelectableByMouse);
	m_text->setWordWrap(true);

	QToolButton* close = new QToolButton(this);
	close->setAutoRaise(true);
	close->setIconSize(QSize(16,16));
	close->setIcon(QIcon::fromTheme("dialog-close"));
	close->setToolTip(tr("Close (%1)").arg(ActionManager::instance()->action("DismissAlert")->shortcut().toString(QKeySequence::NativeText)));
	connect(close, &QToolButton::clicked, this, &Alert::fadeOut);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(7, 7, 7, 7);
	layout->setSpacing(6);
	layout->addWidget(m_expander, 0, Qt::AlignHCenter | Qt::AlignBottom);
	layout->addWidget(m_pixmap);
	layout->addWidget(m_text, 1);
	layout->addWidget(close, 0, Qt::AlignHCenter | Qt::AlignTop);

	m_fade_effect = new QGraphicsOpacityEffect(this);
	m_fade_effect->setOpacity(0.0);
	setGraphicsEffect(m_fade_effect);

	m_fade_timer = new QTimeLine(240, this);
	connect(m_fade_timer, &QTimeLine::valueChanged, m_fade_effect, &QGraphicsOpacityEffect::setOpacity);
}

//-----------------------------------------------------------------------------

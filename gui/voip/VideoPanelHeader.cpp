#include "stdafx.h"
#include "VideoPanelHeader.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../utils/utils.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "VoipTools.h"
#include "../controls/TextEmojiWidget.h"
#include "NameAndStatusWidget.h"

#define SCALED_VALUE(x) (Utils::scale_value((x)))
#define DEFAULT_BORDER SCALED_VALUE(12)
#define DEFAULT_TIME_BUTTONS_OFFSET SCALED_VALUE(300)

std::string Ui::getFotmatedTime(unsigned ts) {
    int h = ts / (60 * 60);
    int m = (ts / 60) % 60;
    int s = ts % 60;

    std::stringstream ss;
    if (h > 0) {
        ss << std::setfill('0') << std::setw(2) << h << ":";
    }
    ss << std::setfill('0') << std::setw(2) << m << ":";
    ss << std::setfill('0') << std::setw(2) << s;

    return ss.str();
}

Ui::MoveablePanel::MoveablePanel(QWidget* parent)
: QWidget(NULL)
, _parent(parent) {
	_drag_state.is_draging = false;
}

Ui::MoveablePanel::~MoveablePanel() {
}

void Ui::MoveablePanel::mouseReleaseEvent(QMouseEvent* /*e*/) {
	_drag_state.is_draging = false;
}

void Ui::MoveablePanel::mousePressEvent(QMouseEvent* e) {
	if (_parent && !_parent->isFullScreen()) {
		_drag_state.is_draging = true;
		_drag_state.pos_drag_begin = e->pos();
	}
}

void Ui::MoveablePanel::mouseMoveEvent(QMouseEvent* e) {
    if ((e->buttons() & Qt::LeftButton) && _drag_state.is_draging) {
        QPoint diff = e->pos() - _drag_state.pos_drag_begin;
        QPoint newpos = _parent->pos() + diff;

        _parent->move(newpos);
    }
}

void Ui::MoveablePanel::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);

    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow() || uiWidgetIsActive()) {
            if (_parent) {
                _parent->blockSignals(true);
                _parent->raise();
                raise();
                _parent->blockSignals(false);
            }
        }
    }
}

void Ui::MoveablePanel::keyReleaseEvent(QKeyEvent* e) {
    QWidget::keyReleaseEvent(e);
    if (e->key() == Qt::Key_Escape) {
        emit onkeyEscPressed();
    }
}

bool Ui::VideoPanelHeader::uiWidgetIsActive() const {
    return _lowWidget->isActiveWindow();
}

void Ui::VideoPanelHeader::_onMinimize() {
    emit onMinimize();
}

void Ui::VideoPanelHeader::_onMaximize() {
    emit onMaximize();
}

void Ui::VideoPanelHeader::_onClose() {
    emit onClose();
}

Ui::VideoPanelHeader::VideoPanelHeader(QWidget* parent, int items)
: MoveablePanel(parent)
, _callName(NULL)
, _lowWidget(NULL)
, _callTime(NULL)
, _btnMin(NULL)
, _btnMax(NULL)
, _btnClose(NULL)
, _callNameSpacer(NULL)
, _callTimeSpacer(NULL)
, _items_to_show(items) {
    setProperty("VideoPanelHeader", true);
    setContentsMargins(0, 0, 0, 0);
    setAttribute(Qt::WA_ShowWithoutActivating);

    QHBoxLayout* layout = new QHBoxLayout();
    _lowWidget = new QWidget(this);
    { // low widget. it makes background panel coloured
        _lowWidget->setContentsMargins(0, 0, 0, 0);
        _lowWidget->setProperty("VideoPanelHeader", true);
        _lowWidget->setLayout(layout);

        QVBoxLayout* vLayoutParent = new QVBoxLayout();
        vLayoutParent->setContentsMargins(0, 0, 0, 0);
        vLayoutParent->setSpacing(0);
        vLayoutParent->setAlignment(Qt::AlignVCenter);
        vLayoutParent->addWidget(_lowWidget);
        setLayout(vLayoutParent);

        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setAlignment(Qt::AlignVCenter);

        layout->addSpacing(DEFAULT_BORDER);
    }

    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);
    if (_items_to_show & kVPH_ShowName) {
        _callName = new NameWidget(_lowWidget, Utils::scale_value(15));
        _callName->setFont(font);
        _callName->layout()->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        _callName->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        _callName->setNameProperty("VideoPanelHeaderText", true);

        layout->addWidget(_callName);
        layout->addSpacing(DEFAULT_BORDER << 1);
    }

    if (_items_to_show & kVPH_ShowTime) {
        _callTime = new QLabel(_lowWidget);
        _callTime->setFont(font);
        _callTime->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        _callTime->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
        _callTime->setProperty("VideoPanelHeaderText", true);
        layout->addWidget(_callTime);
        layout->addSpacing(DEFAULT_BORDER * 3);
    }

    QWidget* parentWidget = _lowWidget;
    auto __addButton = [this, parentWidget, layout] (const char* propertyName, const char* slot)->QPushButton* {
        QPushButton* btn = new QPushButton(parentWidget);
        btn->setProperty(propertyName, true);
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);
        layout->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, slot, Qt::QueuedConnection);
        return btn;
    };

    bool spacerBetweenBtnsAndTimeAdded = false;
    if (_items_to_show & kVPH_ShowMin) {
        if (!spacerBetweenBtnsAndTimeAdded) {
            _callTimeSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
            layout->addSpacerItem(_callTimeSpacer);
            spacerBetweenBtnsAndTimeAdded = true;
        }
        _btnMin = __addButton("VideoPanelHeaderMin", SLOT(_onMinimize()));
    }

    if (_items_to_show & kVPH_ShowMax) {
        if (!spacerBetweenBtnsAndTimeAdded) {
            _callTimeSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
            layout->addSpacerItem(_callTimeSpacer);
            spacerBetweenBtnsAndTimeAdded = true;
        }
        _btnMax = __addButton("VideoPanelHeaderMax", SLOT(_onMaximize()));
    }

    if (_items_to_show & kVPH_ShowClose) {
        if (!spacerBetweenBtnsAndTimeAdded) {
            _callTimeSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
            layout->addSpacerItem(_callTimeSpacer);
            spacerBetweenBtnsAndTimeAdded = true;
        }
        _btnClose = __addButton("VideoPanelHeaderClose", SLOT(_onClose()));
    }
}

Ui::VideoPanelHeader::~VideoPanelHeader() {
    
}

void Ui::VideoPanelHeader::resizeEvent(QResizeEvent* e) {
	QWidget::resizeEvent(e);

    if (!_callName || !_callTime) {
        return;
    }

    _callName->show();
}

void Ui::VideoPanelHeader::enterEvent(QEvent* e) {
	QWidget::enterEvent(e);
	emit onMouseEnter();
}

void Ui::VideoPanelHeader::leaveEvent(QEvent* e) {
	QWidget::leaveEvent(e);
	emit onMouseLeave();
}

void Ui::VideoPanelHeader::setCallName(const std::string& name) {
    if (!!_callName) {
        _callName->setName(name.c_str());
    }
}

void Ui::VideoPanelHeader::setTime(unsigned ts, bool have_call) {
    if (!have_call) {
        if (!!_callTime) {
            _callTime->setText("");
        }
	} else {
        if (!!_callTime) {
            _callTime->setText(getFotmatedTime(ts).c_str());
        }
	}
}

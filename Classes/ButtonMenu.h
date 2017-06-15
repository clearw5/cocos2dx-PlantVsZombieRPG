#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"
using namespace std;
USING_NS_CC;
using namespace cocos2d::ui;

class ButtonMenuBuilder {
public:

	static Sprite *createShadow(const Size &size, const Vec2 &pos, int opacity = 180) {
		auto shadow = Sprite::create();
		shadow->setColor(Color3B::BLACK);
		shadow->setOpacity(opacity);
		shadow->setTextureRect(Rect(0, 0, size.width, size.height));
		shadow->setContentSize(size);
		shadow->setPosition(pos);
		return shadow;
	}

	ButtonMenuBuilder() {

	}
	ButtonMenuBuilder &addItem(const string &title, const Widget::ccWidgetClickCallback& callback) {
		_items.push_back(title);
		_callbacks.push_back(callback);
		return *this;
	}
	
	ButtonMenuBuilder &font(const string &_font) {
		this->_font = _font;
		return *this;
	}
	ButtonMenuBuilder &fontSize(int _fontSize) {
		this->_fontSize = _fontSize;
		return *this;
	}
	ButtonMenuBuilder &margin(int _margin) {
		this->_margin = _margin;
		return *this;
	}
	ButtonMenuBuilder &horizontal(bool _horizontal) {
		this->_horizontal = _horizontal;
		return *this;
	}
	ButtonMenuBuilder &autoDismiss(bool _autoDismiss) {
		this->_autoDismiss = _autoDismiss;
		return *this;
	}
	ButtonMenuBuilder &onDismiss(const Widget::ccWidgetClickCallback &_onDismiss) {
		this->_onDismiss = _onDismiss;
		return *this;
	}

	Sprite *build() {
		_menu = Sprite::create();
		float w = 0, h = 0;
		if (_horizontal) {
			w = (_items.size() - 1) * _margin;
		}
		else {
			h = (_items.size() - 1) * _margin;
		}
		for (size_t i = 0; i < _items.size(); i++) {
			auto button = Button::create();
			button->setTitleText(_items[i]);
			button->setTag(i);
			button->addClickEventListener(_callbacks[i]);
			if (!_font.empty()) {
				button->setTitleFontName(_font);
			}
			if (_fontSize > 0) {
				button->setTitleFontSize(_fontSize);
			}
			_menu->addChild(button);
			if (_horizontal) {
				h = MAX(h, button->getContentSize().height);
				w += button->getContentSize().width;
			}
			else {
				w = MAX(w, button->getContentSize().width);
				h += button->getContentSize().height;
			}
		}
		_menu->setContentSize(Size(w, h));
		auto children = _menu->getChildren();
		if (_horizontal) {
			w = 0;
		}
		else {
			h = 0;
		}
		for (size_t i = 0; i < _items.size(); i++) {
			auto button = (Sprite*) children.at(i);
			if (_horizontal) {
				button->setPosition(w, h / 2);
				w += button->getContentSize().width + _margin;
			}
			else {
				button->setPosition(w / 2, _menu->getContentSize().height - h);
				h += button->getContentSize().height + _margin;
			}
		}
		return _menu;
	}

	void onClick(int i, Ref* ref) {
		if (_autoDismiss) {
			(dynamic_cast<Button*>(ref))->getParent()->removeFromParent();
			_onDismiss(ref);
		}
		_callbacks[i](ref);
	}


private:
	vector<string> _items;
	vector<Widget::ccWidgetClickCallback> _callbacks;
	Widget::ccWidgetClickCallback _onDismiss;
	string _font = "";
	int _fontSize = -1;
	Sprite *_menu = nullptr;
	int _margin = 0;
	bool _autoDismiss = false;
	bool _horizontal = false;
	

};


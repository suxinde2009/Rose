#ifndef SLEEP_UNIT_HPP_INCLUDED
#define SLEEP_UNIT_HPP_INCLUDED

#include "base_unit.hpp"

class chart_controller;
class chart_display;
class unit_map;

class unit: public base_unit
{
public:
	enum {NONE, SNORE, MOTION};

	~unit();

	bool require_sort() const { return true; }
	void redraw_unit();

	int type() const { return type_; }

protected:
	chart_controller& controller_;
	chart_display& disp_;
	unit_map& units_;

	int type_;
};

#endif

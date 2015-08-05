#ifndef CHART_UNIT_MAP_HPP_INCLUDED
#define CHART_UNIT_MAP_HPP_INCLUDED

#include "unit.hpp"
#include "base_map.hpp"

class chart_controller;

class unit_map: public base_map
{
public:
	unit_map(chart_controller& controller, const gamemap& gmap, bool consistent);

	void create_coor_map(int w, int h);
	void add(const map_location& loc, const base_unit* base_u);

	unit* find_unit(const map_location& loc) const;
	unit* find_unit(const map_location& loc, bool verlay) const;

	unit* find_unit(int i) const { return dynamic_cast<unit*>(map_[i]); }

private:
	chart_controller& controller_;
};

#endif

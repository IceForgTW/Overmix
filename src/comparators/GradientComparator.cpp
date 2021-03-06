/*
	This file is part of Overmix.

	Overmix is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Overmix is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Overmix.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "GradientComparator.hpp"

#include <QRect>

using namespace Overmix;


static double calculate_overlap( Point<> offset, const Plane& img1, const Plane& img2 ){
	QRect first( 0,0, img1.get_width(), img1.get_height() );
	QRect second( { offset.x, offset.y }, QSize(img2.get_width(), img2.get_height()) );
	QRect common = first.intersected( second );
	
	double area = first.width() * first.height();
	return (double)common.width() * common.height() / area;
}

ImageOffset GradientComparator::findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2 ) const{
	Point<double> moves{ method == AlignMethod::VER ? 0.0 : movement
	                   , method == AlignMethod::HOR ? 0.0 : movement
	                   };
	
	std::pair<Point<>,double> result;
	DiffCache cache;
	int level = start_level;
	
	//Keep repeating with higher levels until it drops below threshold
	do{
		result = img1.best_round_sub( img2
			,	a1, a2, level
			,	((int)1 - (int)img2.get_width() ) * moves.x, ((int)img1.get_width()  - 1) * moves.x
			,	((int)1 - (int)img2.get_height()) * moves.y, ((int)img1.get_height() - 1) * moves.y
			,	&cache, fast_diffing
			);
	}while( result.second > max_difference && level++ < max_level );
	
	return { result.first, result.second, calculate_overlap( result.first, img1, img2 ) };
}


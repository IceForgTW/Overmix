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

#include "Plane.hpp"
#include <algorithm> //For min
#include <cstdint> //For abs(int) and uint*_t
#include <limits>
#include <vector>

#include <QtConcurrent>
#include <QDebug>

const unsigned long Plane::MAX_VAL = 0xFFFFFFFF;
static const double DOUBLE_MAX = std::numeric_limits<double>::max();

Plane::Plane( unsigned w, unsigned h ){
	height = h;
	width = w;
	line_width = w;
	data = new color_type[ h * line_width ];
}

Plane::~Plane(){
	if( data )
		delete[] data;
}

bool Plane::is_interlaced() const{
	double avg2 = 0;
	for( unsigned iy=0; iy<get_height()/2*2; ){
		color_type *row1 = scan_line( iy++ );
		color_type *row2 = scan_line( iy++ );
		
		unsigned long line_avg = 0;
		for( unsigned ix=0; ix<get_width(); ++ix ){
			color_type diff = row2[ix] > row1[ix] ? row2[ix] - row1[ix] : row1[ix] - row2[ix];
			line_avg += (unsigned long)diff*diff;
		}
		avg2 += (double)line_avg / get_width();
	}
	avg2 /= get_height()/2;
	avg2 /= 0xFFFF;
	avg2 /= 0xFFFF;
	
	qDebug( "interlace factor: %f", avg2 );
	return avg2 > 0.0015; //NOTE: Based on experiments, not reliable!
	//TODO: diff even with even and compare againts that?
}

void Plane::replace_line( Plane &p, bool top ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "replace_line: Planes not equaly sized!" );
		return;
	}
	
	for( unsigned iy=(top ? 0 : 1); iy<get_height(); iy+=2 ){
		color_type *row1 = scan_line( iy );
		color_type *row2 = p.scan_line( iy );
		
		for( unsigned ix=0; ix<get_width(); ++ix )
			row1[ix] = row2[ix];
	}
}

void Plane::combine_line( Plane &p, bool top ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "combine_line: Planes not equaly sized!" );
		return;
	}
	
	for( unsigned iy=(top ? 0 : 1); iy<get_height(); iy+=2 ){
		color_type *row1 = scan_line( iy );
		color_type *row2 = p.scan_line( iy );
		
		for( unsigned ix=0; ix<get_width(); ++ix )
			row1[ix] = ( (unsigned)row1[ix] + row2[ix] ) / 2;
	}
}


struct Sum{
	uint64_t total;
	Sum() : total( 0 ) { }
	void reduce( const uint64_t add ){ total += add; }
};
struct Para{
	color_type *c1;
	color_type *c2;
	unsigned width;
	unsigned stride;
	Para( color_type* c1, color_type *c2, unsigned width, unsigned stride )
		:	c1( c1 ), c2( c2 ), width( width ), stride( stride )
		{ }
};
static uint64_t diff_2_line( Para p ){
	uint64_t sum = 0;
	for( color_type* end=p.c1+p.width; p.c1<end; p.c1+=p.stride, p.c2+=p.stride )
		sum += std::abs( *p.c1 - *p.c2 );
	
	return sum;
}

double Plane::diff( const Plane& p, int x, int y, unsigned stride ) const{
	//Find edges
	int p1_top = y < 0 ? 0 : y;
	int p2_top = y > 0 ? 0 : -y;
	int p1_left = x < 0 ? 0 : x;
	int p2_left = x > 0 ? 0 : -x;
	unsigned width = std::min( get_width() - p1_left, p.get_width() - p2_left );
	unsigned height = std::min( get_height() - p1_top, p.get_height() - p2_top );
	
	//Initial offsets on the two planes
	color_type* c1 = scan_line( p1_top ) + p1_left;
	color_type* c2 = p.scan_line( p2_top ) + p2_left;
	
	//Calculate all the offsets for QtConcurrent::mappedReduced
	std::vector<Para> lines;
	lines.reserve( height );
	for( unsigned i=0; i<height; i+=stride ){
		lines.push_back( Para( c1+i%stride, c2+i%stride, width, stride ) );
		c1 += line_width;
		c2 += p.line_width;
	}
	
	Sum sum = QtConcurrent::blockingMappedReduced( lines, &diff_2_line, &Sum::reduce );
	return sum.total / (double)( height * width / (stride*stride) );
}


#include <QTime>
Plane* Plane::scale_nearest( unsigned wanted_width, unsigned wanted_height ) const{
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return 0;
	
	for( unsigned iy=0; iy<wanted_height; iy++ ){
		color_type* row = scaled->scan_line( iy );
		for( unsigned ix=0; ix<wanted_width; ix++ ){
			double pos_x = ((double)ix / (wanted_width-1)) * (width-1);
			double pos_y = ((double)iy / (wanted_height-1)) * (height-1);
			
			row[ix] = pixel( round( pos_x ), round( pos_y ) );
		}
	}
	
	return scaled;
}

double Plane::linear( double x ){
	x = std::abs( x );
	if( x <= 1.0 )
		return x;
	return 0;
}

double Plane::cubic( double b, double c, double x ){
	x = std::abs( x );
	
	if( x < 1 )
		return
				(12 - 9*b - 6*c)/6 * x*x*x
			+	(-18 + 12*b + 6*c)/6 * x*x
			+	(6 - 2*b)/6
			;
	else if( x < 2 )
		return
				(-b - 6*c)/6 * x*x*x
			+	(6*b + 30*c)/6 * x*x
			+	(-12*b - 48*c)/6 * x
			+	(8*b + 24*c)/6
			;
	else
		return 0;
}


struct ScalePoint{
	unsigned start;
	std::vector<double> weights; //Size of this is end
	
	ScalePoint( unsigned index, unsigned width, unsigned wanted_width, double window, Plane::Filter f ){
		using namespace std;
		
		double pos = ((double)index / (wanted_width-1)) * (width-1);
		start = (unsigned)max( (int)ceil( pos-window ), 0 );
		unsigned end = min( (unsigned)floor( pos+window ), width-1 );
		
		weights.reserve( end - start + 1 );
		for( unsigned j=start; j<=end; ++j )
			weights.push_back( f( pos - j ) );
	}
};

struct ScaleLine{
	std::vector<ScalePoint>& points;
	Plane& wanted;
	unsigned index;
	unsigned width;
	
	color_type* row; //at (0,index)
	unsigned line_width;
	double window;
	Plane::Filter f;
	
	ScaleLine( std::vector<ScalePoint>& points, Plane &wanted, unsigned index, unsigned width )
		:	points(points), wanted(wanted), index(index), width(width) { }
};

void do_line( const ScaleLine& line ){
	color_type *out = line.wanted.scan_line( line.index );
	ScalePoint ver( line.index, line.width, line.wanted.get_height(), line.window, line.f );
	
	for( auto x : line.points ){
		double avg = 0;
		double amount = 0;
		color_type* row = line.row - (line.index-ver.start)*line.line_width + x.start;
		
		for( auto wy : ver.weights ){
			color_type* row2 = row;
			for( auto wx : x.weights ){
				double weight = wy * wx;
				avg += *(row2++) * weight;
				amount += weight;
			}
			
			row += line.line_width;
		}
		
		//TODO: limit negative numbers
		*(out++) = amount ? std::min( unsigned( avg / amount + 0.5 ), 0xFFFFu ) : 0;
		
	}
}

Plane* Plane::scale_generic( unsigned wanted_width, unsigned wanted_height, double window, Plane::Filter f ) const{
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return 0;
	
	QTime t;
	t.start();
	
	//Calculate all x-weights
	std::vector<ScalePoint> points;
	points.reserve( wanted_width );
	for( unsigned ix=0; ix<wanted_width; ++ix ){
		ScalePoint p( ix, width, wanted_width, window, f );
		points.push_back( p );
	}
	
	//Calculate all y-lines
	std::vector<ScaleLine> lines;
	for( unsigned iy=0; iy<wanted_height; ++iy ){
		ScaleLine line( points, *scaled, iy, height );
		line.row = scan_line( iy );
		line.line_width = line_width;
		line.window = window;
		line.f = f;
		lines.push_back( line );
	}
	
	QtConcurrent::blockingMap( lines, &do_line );
	
	qDebug( "Resize took: %d msec", t.restart() );
	return scaled;
}



//TODO: these two are brutally simple, improve?
double DiffCache::get_diff( int x, int y ) const{
	for( auto c : cache )
		if( c.x == x && c.y == y ){
		//	qDebug( "Reusing diff: %dx%d with %.2f", x, y, c.diff );
			return c.diff;
		}
	return -1;
}
void DiffCache::add_diff( int x, int y, double diff ){
	Cached c = { x, y, diff };
	cache << c;
}



struct img_comp{
	const Plane& img1;
	const Plane& img2;
	int h_middle;
	int v_middle;
	double diff;
	int level;
	int left;
	int right;
	int top;
	int bottom;
	bool diff_set;
	
	img_comp( const Plane& image1, const Plane& image2, int hm, int vm, int lvl=0, int l=0, int r=0, int t=0, int b=0 )
		:	img1( image1 ), img2( image2 )
		,	h_middle( hm ), v_middle( vm )
		,	diff( -1 )
		,	level( lvl )
		,	left( l ), right( r )
		,	top( t ), bottom( b )
		,	diff_set( false )
		{ }
	void do_diff( int x, int y ){
		if( !diff_set )
			diff = img1.diff( img2, x, y );
	}
	void set_diff( double new_diff ){
		diff = new_diff;
		if( diff >= 0 )
			diff_set = true;
	}
	
	MergeResult result( DiffCache *cache ) const{
		if( level > 0 )
			return img1.best_round_sub( img2, level, left, right, top, bottom, cache );
		else
			return MergeResult(QPoint( h_middle, v_middle ),diff);
	}
};

static void do_diff_center( img_comp& comp ){
	comp.do_diff( comp.h_middle, comp.v_middle );
}
MergeResult Plane::best_round_sub( const Plane& p, int level, int left, int right, int top, int bottom, DiffCache *cache ) const{
//	qDebug( "Round %d: %d,%d x %d,%d", level, left, right, top, bottom );
	std::vector<img_comp> comps;
	int amount = level*2 + 2;
	double h_offset = (double)(right - left) / amount;
	double v_offset = (double)(bottom - top) / amount;
	level = level > 1 ? level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		//Handle trivial step
		//Check every diff in the remaining area
		for( int ix=left; ix<=right; ix++ )
			for( int iy=top; iy<=bottom; iy++ ){
				img_comp t( *this, p, ix, iy );
				t.set_diff( cache->get_diff( ix, iy ) );
				comps.push_back( t );
			}
	}
	else{
		//Make sure we will not do the same position multiple times
		double h_add = ( h_offset < 1 ) ? 1 : h_offset;
		double v_add = ( v_offset < 1 ) ? 1 : v_offset;
		
		for( double iy=top+v_offset; iy<=bottom; iy+=v_add )
			for( double ix=left+h_offset; ix<=right; ix+=h_add ){
				int x = ( ix < 0.0 ) ? ceil( ix-0.5 ) : floor( ix+0.5 );
				int y = ( iy < 0.0 ) ? ceil( iy-0.5 ) : floor( iy+0.5 );
				
				//Avoid right-most case. Can't be done in the loop
				//as we always want it to run at least once.
				if( ( x == right && x != left ) || ( y == bottom && y != top ) )
					continue;
				
				//Create and add
				img_comp t(
						*this, p, x, y, level
					,	floor( ix - h_offset ), ceil( ix + h_offset )
					,	floor( iy - v_offset ), ceil( iy + v_offset )
					);
				
				t.set_diff( cache->get_diff( x, y ) );
				
				comps.push_back( t );
			}
	}
	
	//Calculate diffs
	QtConcurrent::map( comps, do_diff_center ).waitForFinished();
	
	//Find best comp
	const img_comp* best = NULL;
	double best_diff = DOUBLE_MAX;
	
	for( auto &c : comps ){
		if( c.diff < best_diff ){
			best = &c;
			best_diff = best->diff;
		}
		
		//Add to cache
		if( !c.diff_set )
			cache->add_diff( c.h_middle, c.v_middle, c.diff );
	}
	
	if( !best ){
		qDebug( "ERROR! no result to continue on!!" );
		return MergeResult(QPoint(),DOUBLE_MAX);
	}
	
	return best->result( cache );
}




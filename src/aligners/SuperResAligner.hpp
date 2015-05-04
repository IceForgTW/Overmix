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

#ifndef SUPER_RES_ALIGNER_HPP
#define SUPER_RES_ALIGNER_HPP

#include "AImageAligner.hpp"

class SuperResAligner : public AImageAligner{
	private:
		double local_scale;
	public:
		SuperResAligner( AContainer& container, AlignMethod method, double scale=1.0 )
			:	AImageAligner( container, method, 1.0 ), local_scale( scale ) { }
		virtual void align( AProcessWatcher* watcher=nullptr ) override;
};

#endif
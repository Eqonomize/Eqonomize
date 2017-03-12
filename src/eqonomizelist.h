/***************************************************************************
 *   Copyright (C) 2017 by Hanna Knutsson                                  *
 *   hanna_k@fmgirl.com                                                    *
 *                                                                         *
 *   This file is part of Eqonomize!.                                      *
 *                                                                         *
 *   Eqonomize! is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Eqonomize! is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Eqonomize!. If not, see <http://www.gnu.org/licenses/>.    *
 ***************************************************************************/

#ifndef EQONOMIZE_LIST_H
#define EQONOMIZE_LIST_H

#include <QList>

template<class type> class EqonomizeList : public QList<type> {
	protected:
		bool b_auto_delete;
		int i_index;
	public:
		EqonomizeList() : QList<type>(), b_auto_delete(false), i_index(0) {};
		virtual ~EqonomizeList () {}
		void setAutoDelete(bool b) {
			b_auto_delete = b;
		}
		void clear() {
			if(b_auto_delete) {
				while(!QList<type>::isEmpty()) delete QList<type>::takeFirst();
			} else {
				QList<type>::clear();
			}
		}		
		bool removeRef(type value) {
			if(b_auto_delete) delete value;
			return QList<type>::removeOne(value);
		}		
		type first() {
			i_index = 0;
			if(QList<type>::isEmpty()) return NULL;
			return QList<type>::first();
		}
		type last() {
			i_index = QList<type>::count();
			if(QList<type>::isEmpty()) return NULL;
			i_index--;
			return QList<type>::at(i_index);
		}
		type getFirst() {
			return QList<type>::first();
		}
		type next() {
			i_index++;
			if(i_index >= QList<type>::count()) return NULL;
			return QList<type>::at(i_index);
		}
		type current() {
			if(i_index >= QList<type>::count()) return NULL;
			return QList<type>::at(i_index);
		}
		type previous() {			
			if(i_index == 0) return NULL;
			i_index--;
			return QList<type>::at(i_index);
		}
};

#endif

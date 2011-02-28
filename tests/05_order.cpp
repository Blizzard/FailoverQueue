
#include "FailoverQueue.hpp"

/*
** Copyright (c) 2010-2011 Blizzard Entertainment
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
*/

#include <boost/shared_ptr.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#define TEST_PATH "./"

using namespace std;

class Counter {
public:
	Counter() : count_(0) { }
	~Counter() { }
	int count() { return count_; }
	void count(int count) { count_ = count; }
private:
	friend class boost::serialization::access;

	int count_;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int /* version */) {
		ar & count_;
	}
};

typedef boost::shared_ptr<Counter> CounterPtr;

void reset() {
	if (!boost::filesystem::is_directory(TEST_PATH)) {
		return;
	}
	boost::filesystem::directory_iterator end_iter;
	for (boost::filesystem::directory_iterator dir_itr(TEST_PATH); dir_itr != end_iter; ++dir_itr) {
		if (boost::filesystem::is_regular_file(dir_itr->status())) {
			std::string fileName = dir_itr->path().filename();
			if (fileName.find("failover") == 0) {
				boost::filesystem::remove(dir_itr->path().filename());
			}
		}
	}
}

int main() {
	reset();
	FailoverQueue<Counter, CounterPtr> counterQueue(TEST_PATH, 10);
	assert(counterQueue.size() == 0);

	for (int i = 0; i < 20; i++) {
		CounterPtr cptr(new Counter());
		cptr->count(i);
		counterQueue.push(cptr);
	}

	int order[] = {10,11,12,13,14,15,16,17,0,1,2,3,4,5,6,7,8,9,18,19};
	list<int> orderList(order, order + sizeof(order) / sizeof(int));

	for (list<int>::iterator it = orderList.begin(); it != orderList.end(); it++) {
		CounterPtr cptr = counterQueue.popw();
		assert(cptr->count() == *it);
	}
	return 0;
}

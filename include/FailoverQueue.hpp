#ifndef __FAILOVERQUEUE_H__
#define __FAILOVERQUEUE_H__

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

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <fstream>
#include <string>
#include <sstream>
#include <string>
#include <queue>
#include <vector>
#include <cstring>

/*! \def FQ_FILENAME
*** The prefix used when creating and reading failover files.
**/
#ifndef FQ_FILENAME
#define FQ_FILENAME "failover"
#endif

/*! \def FQ_EXT
*** The extension used when creating and reading failover files.
**/
#ifndef FQ_EXT
#define FQ_EXT ".log"
#endif

/*! \def FQ_MIN_SIZE(N)
*** A function that returns the low-threshold point at which failover files are read.
**/
#ifndef FQ_MIN_SIZE
#define FQ_MIN_SIZE(N) N * 0.25
#endif

/*! \def FQ_DUMP_SIZE(N)
*** A function that returns the number of items to put into a failover file.
**/
#ifndef FQ_DUMP_SIZE
#define FQ_DUMP_SIZE(N) N / 2
#endif

#define DEBUG 1

#ifdef DEBUG
#include <iostream>
#define FQD(X) std::cout << __FILE__ << ":" << __LINE__ << " " << X << std::endl;
#else
#define FQD(X)
#endif

/*!
 * \class FailoverQueue
 * \brief A thread-safe queue-like container that will spill-over into files on disk.
 *
 * This project provides a thread-safe queue-like container that has the ability
 * to spill queue content into failover files for later reprocessing. This
 * allows the queus to be fixed sizes (queue length) that periodically write
 * overflow items into files that can be read back and inserted into the queue
 * when the queue has fewer items.
 *
 * \section Configuration
 * The following define macros can be used to change the behavior of the class
 * if they are defined before the header file is included.
 * \li FQ_FILENAME()
 * \li FQ_EXT()
 * \li FQ_MIN_SIZE(N)
 * \li FQ_DUMP_SIZE(N)
 * 
 *  \author Nick Gerakines <ngerakines@blizzard.com>
 *  \version 0.2.0
**/
template <class BaseClass, class BaseClassPointer>
class FailoverQueue {
	private:
		std::queue<BaseClassPointer> theQueue_;

		int maxBucket_;
		mutable boost::mutex mutex_;
		boost::condition_variable condition_;
		int itemCount_;

		int maxSize_;

		std::vector<std::string> failOverFiles_;
		int failOverCount_;
		double minCount_;

		std::string failOverPath_;

	public:
		//! Construct a failover queue object with a given path and max size.
		/*! \param path The directory that failover files are saved in.
		 *  \param maxSize The queue item size that must be reached before items are dumped into failover files.
		**/
		FailoverQueue(std::string path, int maxSize) : maxBucket_(0), itemCount_(0), maxSize_(maxSize), failOverCount_(0), failOverPath_(path) {
			minCount_ = FQ_MIN_SIZE(maxSize_);

			bootstrap();
		}

		//! The deconstructor
		~FailoverQueue() {
			condition_.notify_all();
		}

		//! Returns true if the internal queue is empty.
		/*! It is important to note that this does not account for the
		 *  the existance or item count of failover files. It is
		 *  important to note that if the FQ_MIN_SIZE(N) macro is set to
		 *  return 0, a race condition could occur where this method
		 *  return true while a failover file is being read.
		**/
		bool empty() {
			boost::mutex::scoped_lock lock(mutex_);
			return itemCount_ == 0;
		}

		//! Returns the size of the internal queue.
		/*! This method does not take into account the number of or size
		 *  of any failoever files managed by this object.
		**/
		int size() {
			boost::mutex::scoped_lock lock(mutex_);
			return itemCount_;
		}

		//! Adds an item to the queue.
		bool push(BaseClassPointer item) {
			boost::mutex::scoped_lock lock(mutex_);
			if (itemCount_ > maxSize_) {
				std::string fileName = failOverFile();
				std::ofstream ofs(fileName.c_str());
				boost::archive::text_oarchive oa(ofs);
				int c = FQ_DUMP_SIZE(maxSize_);
				fq_container container;
				while (c > 0) {
					BaseClassPointer tmpItem = theQueue_.front();
					container.add(*tmpItem.get());
					theQueue_.pop();
					--itemCount_;
					--c;
				}
				oa << container;
			}
			theQueue_.push(item);
			++itemCount_;
			condition_.notify_one();
			return true;
		}

		//! Pops an item from the queue, waiting until one is available if necessary.
		/*! This is a blocking operation.
		**/
		BaseClassPointer popw() {
			boost::mutex::scoped_lock lock(mutex_);

			while (!fill());

			while (theQueue_.empty() && maxSize_ != -1 && failOverCount_ < 1) {
				condition_.wait(lock);
			}

			BaseClassPointer item = theQueue_.front();
			theQueue_.pop();
			--itemCount_;
			return item;
		}

		//! Empties the internal queue and optionally deletes all of the failover files.
		void clear(bool deleteFiles = true) {
			boost::mutex::scoped_lock lock(mutex_);
			itemCount_ = 0;
			while (!theQueue_.empty()) {
				theQueue_.pop();
			}
			maxSize_ = -1;
			if (deleteFiles) {
				for (int i = 0; i < (int) failOverFiles_.size(); i++) {
					std::string fileName = failOverFiles_[i];
				}
			}
			// NKG: Should this be notify_all if there are more than one blocked requestors of popw()?
			condition_.notify_one();
		}

		//! Returns the known failover files.
		std::vector<std::string> failOverFiles() { return failOverFiles_; }

	private:

		/*! \brief A private utility class used to create variable-length failover files.
		 *  \private
		**/
		class fq_container {
			public:
				std::vector<BaseClass> data;
				void add(BaseClass item) {
					data.push_back(item);
				}
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive & ar, const unsigned int /* version */) {
					ar & data;
				}
		};

		/*! \brief Attempt to determine if a failover file needs to be read and so.
		 *  \private
		**/
		bool fill() {
			if (itemCount_ > minCount_ || failOverCount_ == 0) {
				return true;
			}
			std::string fileName = nextFailOverFile();
			FQD("Loading: " << fileName)

			boost::filesystem::path failOverFile(fileName);
			if (!boost::filesystem::exists(failOverFile)) {
				return false;
			}
			std::ifstream ifs(fileName.c_str());
			boost::archive::text_iarchive ia(ifs);
			fq_container container;
			ia >> container;
			std::queue<BaseClassPointer> newQueue;
			for (int i = 0; i < (int) container.data.size(); i++) {
				BaseClassPointer item(new BaseClass(container.data[i]));
				newQueue.push(item);
				++itemCount_;
			}
			while (!theQueue_.empty()) {
				newQueue.push(theQueue_.front());
				theQueue_.pop();
			}
			theQueue_ = newQueue;
			deleteFile(fileName);
			condition_.notify_one();
			return true;
		}

		/*! \brief Discover any existing failover files.
		 *  \private
		**/
		void bootstrap() {
			if (!boost::filesystem::is_directory(failOverPath_)) {
				return;
			}
			std::vector<std::string> tmpList;
			boost::filesystem::directory_iterator end_iter;
			for (boost::filesystem::directory_iterator dir_itr(failOverPath_); dir_itr != end_iter; ++dir_itr) {
				if (boost::filesystem::is_regular_file(dir_itr->status())) {
					std::string fileName = dir_itr->path().filename();
					if (fileName.find(FQ_FILENAME) == 0) {
						std::stringstream output;
						++failOverCount_;
						output << failOverPath_ << fileName;
						failOverFiles_.insert(failOverFiles_.begin(), output.str());
					}
				}
			}
			sort(failOverFiles_.begin(), failOverFiles_.end(), &FailoverQueue::failover_compare);
		}

		/*! \brief Create a unique failoever filename to be used.
		 *  \private
		**/
		std::string failOverFile() {
			std::stringstream output;
			output << failOverPath_ << FQ_FILENAME << ++failOverCount_ << FQ_EXT;
			failOverFiles_.insert(failOverFiles_.begin(), output.str());
			return output.str();
		}

		/*! \brief Find and remove a failover filename to be used.
		 *  \private
		**/
		std::string nextFailOverFile() {
			std::string next = failOverFiles_.back();
			failOverFiles_.pop_back();
			--failOverCount_;
			return next;
		}

		/*! \brief Safely delete a file.
		 *  \private
		**/
		void deleteFile(std::string fileName) {
			boost::filesystem::path failOverFile(fileName);
			if (boost::filesystem::exists(failOverFile)) {
				boost::filesystem::remove(failOverFile);
			}
		}

		/*! \brief A small compare method used to sort failover files.
		 *  \private
		**/
		static bool failover_compare(std::string first, std::string second) {
			// The + 2 is added to represent the './' prefix to each of these strings.
			std::string stringIdA = first.substr(strlen(FQ_FILENAME) + 2, first.length() - strlen(FQ_EXT));
			std::string stringIdB = second.substr(strlen(FQ_FILENAME) + 2, second.length() - strlen(FQ_EXT));
			int idA = atoi(stringIdA.c_str());
			int idB = atoi(stringIdB.c_str());
			return idA <= idB;
		}
};

#endif

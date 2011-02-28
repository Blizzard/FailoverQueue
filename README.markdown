
# About

This project provides a thread-safe queue-like container that has the ability
to spill queue content into failover files for later reprocessing. This allows
the queues to be fixed in size (queue length) and periodically write overflow
items into files that can be read back and inserted into the queue when the
queue has fewer items.

# Version

failoverqueue-v0.2.0

# Usage

The FailoverQueue container is a single header file that can be included into
existing source files.

## Dependancies

The following libraries are used by this project:

* boost::thread
* boost::serialization
* boost::system
* boost::filesystem

# Testing

Contained within the tests directory are a number of unit tests that stress
basic and complex functionality as well as several edge cases.

 * 01_basic: Verify functionality with basic objects
 * 02_complex: Verify functionality with complex objects
 * 03_uneven: Test ability to read failover files on object construction and dequeue items properly
 * 04_even: Test edge case whereby failover files represent an empty queue but dequeuing must take place
 * 05_order: Verify the ability to load failover files in the order in which they were created
 * 06_missing: Verify that missing bug expected failover files are skipped gracefully.

# Credits

Nick Gerakines <ngerakines@blizzard.com>

# License

Copyright (c) 2010-2011 Blizzard Entertainment

Open sourced under the MIT license. See the included LICENSE file for more
information.


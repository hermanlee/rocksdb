// Copyright (c) 2013, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef STORAGE_ROCKSDB_INCLUDE_TRANSACTION_LOG_ITERATOR_H_
#define STORAGE_ROCKSDB_INCLUDE_TRANSACTION_LOG_ITERATOR_H_

#include "rocksdb/status.h"
#include "rocksdb/types.h"
#include "rocksdb/write_batch.h"
#include <memory>
#include <vector>

namespace rocksdb {

class LogFile;
typedef std::vector<std::unique_ptr<LogFile>> VectorLogPtr;

enum  WalFileType {
  /* Indicates that WAL file is in archive directory. WAL files are moved from
   * the main db directory to archive directory once they are not live and stay
   * there until cleaned up. Files are cleaned depending on archive size
   * (Options::WAL_size_limit_MB) and time since last cleaning
   * (Options::WAL_ttl_seconds).
   */
  kArchivedLogFile = 0,

  /* Indicates that WAL file is live and resides in the main db directory */
  kAliveLogFile = 1
} ;

class LogFile {
 public:
  LogFile() {}
  virtual ~LogFile() {}

  // Returns log file's pathname relative to the main db dir
  // Eg. For a live-log-file = /000003.log
  //     For an archived-log-file = /archive/000003.log
  virtual std::string PathName() const = 0;


  // Primary identifier for log file.
  // This is directly proportional to creation time of the log file
  virtual uint64_t LogNumber() const = 0;

  // Log file can be either alive or archived
  virtual WalFileType Type() const = 0;

  // Starting sequence number of writebatch written in this log file
  virtual SequenceNumber StartSequence() const = 0;

  // Size of log file on disk in Bytes
  virtual uint64_t SizeFileBytes() const = 0;
};

struct BatchResult {
  SequenceNumber sequence = 0;
  std::unique_ptr<WriteBatch> writeBatchPtr;
};

// A TransactionLogIterator is used to iterate over the transactions in a db.
// One run of the iterator is continuous, i.e. the iterator will stop at the
// beginning of any gap in sequences
class TransactionLogIterator {
 public:
  TransactionLogIterator() {}
  virtual ~TransactionLogIterator() {}

  // An iterator is either positioned at a WriteBatch or not valid.
  // This method returns true if the iterator is valid.
  // Can read data from a valid iterator.
  virtual bool Valid() = 0;

  // IsObsolete() returns true if new log files were created. This usually
  // means that the user needs to close the current iterator and create a new
  // one to get the newest updates. It should happen only when mem tables are
  // flushed.
  virtual bool IsObsolete() = 0;

  // Moves the iterator to the next WriteBatch.
  // REQUIRES: Valid() to be true.
  virtual void Next() = 0;

  // Returns ok if the iterator is valid.
  // Returns the Error when something has gone wrong.
  virtual Status status() = 0;

  // If valid return's the current write_batch and the sequence number of the
  // earliest transaction contained in the batch.
  // ONLY use if Valid() is true and status() is OK.
  virtual BatchResult GetBatch() = 0;

  // The read options for TransactionLogIterator.
  struct ReadOptions {
    // If true, all data read from underlying storage will be
    // verified against corresponding checksums.
    // Default: true
    bool verify_checksums_;

    ReadOptions() : verify_checksums_(true) {}

    explicit ReadOptions(bool verify_checksums)
        : verify_checksums_(verify_checksums) {}
  };
};
} //  namespace rocksdb

#endif  // STORAGE_ROCKSDB_INCLUDE_TRANSACTION_LOG_ITERATOR_H_

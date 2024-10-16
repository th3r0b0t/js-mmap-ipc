'use strict'

const { mmapIPC } = require('bindings')('mmap-ipc')
module.exports = mmapIPC;

/**
 * Usage example
 * 
 * Note 1: This module uses flock(2) locks, which are only valid between two different processes;
 * If you run the below example with only one process, you'll be able to acquire write lock even tho you're already holding a read lock!
 * In the above case, you're actually upgrading your previous lock!
 * 
 * Note 2:
 * You should read about race-conditions and deadlock with flock(2) locks, if you hadn't before!
 */

/*
let mmap_id     = "firstBuf";   //This name is used to open the same buffer in different processes
let mmap_size   = 4096;       // aka. 4KB
let lock_page   = true;         // Wether to lock the memory page using mlock(2) MLOCK_ONFAULT (this is complete different thing with read/write locks!)
const buf1 = new mmapIPC(mmap_id, mmap_size, lock_page);

console.log( "Acquire write lock:" );
console.log( buf1.acquireWriteLock() );

console.log( buf1.buffer().write('Ya Heidar madad!', 'ascii') );
console.log( "Finished writing!" );

console.log( "Removing lock..." );
console.log( buf1.removeLock() );

console.log( "Acquire read lock:" );
console.log( buf1.acquireReadLock() );

console.log( buf1.buffer().toString('ascii') );
console.log( "Finished reading!" );

console.log( "Trying to acquire write lock before removing read lock:" );
console.log( buf1.acquireWriteLock() );

console.log( buf1.removeLock() );
console.log( "Removing lock..." );
*/
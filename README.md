# Mmap-IPC for Node.JS <img alt="NPM Version" src="https://img.shields.io/npm/v/mmap-ipc?style=flat&logo=npm&logoColor=cb3837">
### Description:
This module is a native addon providing you with the ability to create a shared memory file and use it as `Napi::Buffer` inside Node.JS!  
It also provides you with a synchronization mechanism using flock to cordinate reads/writes between multiple processes using the same memory region. 
For performance tuning, you are also provided with an option to memory lock the created region using mlock;  

See also: [shm_overview(7)](https://man7.org/linux/man-pages/man7/shm_overview.7.html), [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html), [flock(2)](https://man7.org/linux/man-pages/man2/flock.2.html), [mlock(2)](https://man7.org/linux/man-pages/man2/mlockall.2.html)

### Notes:
1. You should be aware of race condition that can happen when upgrading shared locks, to exclusive locks; If you are not, please look it up!
2. Currently mlock uses `MLOCK_ONFAULT` flag for locking memory pages to avoid filling up your memory.
3. The `LOCK_NB` flag is not used and all locks are blocking at C level.

### Installation:
```bash
npm i mmap-ipc
```

### Usage:
Process #1:
```js
const mmapIPC = require('mmap-ipc')
let mmap_id     = "SharedBuf";	//This name is used to open the same buffer in different processes
let mmap_size   = 4096;			// aka. 4KB
let lock_page   = true;         // Wether to lock the memory page using mlock(2) MLOCK_ONFAULT
const SharedBuf = new mmapIPC(mmap_id, mmap_size, lock_page);

console.log( "Acquire write lock:" );
console.log( SharedBuf.acquireWriteLock() );

console.log( SharedBuf.buffer().write('Message from process #1', 'ascii') );
console.log( "Finished writing!" );

console.log( "Removing lock..." );
console.log( SharedBuf.removeLock() );

console.log( "Acquire read lock:" );
console.log( SharedBuf.acquireReadLock() );

console.log( SharedBuf.buffer().toString('ascii') );
console.log( "Finished reading!" );

console.log( SharedBuf.removeLock() );
console.log( "Removing lock..." );
```
----
Process #2:
```js
const mmapIPC = require('mmap-ipc')
let mmap_id     = "SharedBuf";
let mmap_size   = 4096;
let lock_page   = true;
const SharedBuf = new mmapIPC(mmap_id, mmap_size, lock_page);

console.log( "Acquire read lock:" );
console.log( SharedBuf.acquireReadLock() );

console.log( SharedBuf.buffer().toString('ascii') );
console.log( "Finished reading!" );

console.log( "Acquire write lock:" );
console.log( SharedBuf.acquireWriteLock() );

console.log( SharedBuf.buffer().write('This is process #2, I got your message!', 'ascii') );
console.log( "Finished writing!" );

console.log( "Removing lock..." );
console.log( SharedBuf.removeLock() );
```
###### To test this example, you can use a debugger and run the program step by step in 2 different processes as instructed by comments in the codes above!

### API:
[Constructor] `mmapIPC(shm_name, size, lock_memory)` => `Buffer`
##### Return value:
On succuss return a new Node.JS `Buffer` Using the specified shared memory as backing store!  
**Please note**: Shared memory will be created and truncated to the specified size (see `size` param below) if doesn't exist; If the specified shared memory already exists, it'll be open and won't be truncated!  
It's your responsibily to provide the constructor with the correct `size` (or a `size` less then the actual `size` of already existing shared memory) as it will not be truncated if already exist!
##### Params:
- `String shm_name`: A unique name used to identify and map a shared memory region between different processes
- `Numeric size`: Size for the shared memory to be mapped; This is also the size of the returned `Buffer`. If a shared memory does not exist with the given name, a new one will be created, otherwise the already existing one will be mapped, but only as much bytes as specified with this option!
- `Bool lock_memory`: Wether to lock the created/opened memory region with mlock(2). (see the note #2 above)  

----

[Method] `acquireReadLock()` => `Bool`
##### Return value:
Returns `true` if acquiring the shared lock was successful, and will be blocked if needed until lock can be acquired!
##### Params:
This method accepts no parameter.  

----

[Method] `acquireWrireLock()` => `Bool`
##### Return value:
Returns `true` if acquiring the exclusive lock was successful, and will be blocked if needed until lock can be acquired!  
If a shared lock is being held by a previous call to `acquireWrireLock()`, this call will upgrade that lock. (read more at the "see also" section above)
##### Params:
This method accepts no parameter.  

----

[Method] `removeLock()` => `Bool`
##### Return value:
Returns `true` when the job's done!
A call to this method will release any lock that is currently being held.
##### Params:
This method accepts no parameter.  

### Contact infos:
In case of any bug/question/suggestion please fill up an issue on Github.  
Humbly yours, Th3R0b0t.
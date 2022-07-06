// concurrency primitives, including mutexes
// a raft of reasons for this:
// - return EINTR when interrupted
// mutexes:
//  - fast timedlock and recursive mutexes on macos
//  - robust by default
//    - various edge cases like lock on one thread/release on another, or acquire a lock you already hold, are UB in posix!
//  - mutex requisition can be associated with a task, rather than a thread
// condvars:
//  - don't reacquire mutex on wake (_very_ slow)
//  - wake n on linux
// novel primitives, faster than would be possible with pthreads
//  - mutex tokens
//  - queue
//  - pyx

// mutexs are loosely modeled after ulrich drepper 'futexes are tricky'

// todo better error-checking.  In particular:
// - Token should be a task id, not a thread it
// - Error for a task to exit without releasing all its mutexes (how should this be reported to openers of its pyx if the task was in error?)
// - Attempt to detect deadlock, perhaps in a debug mode of some sort (cf freebsd kernel WITNESS)

#include"j.h"

// timing shenanigans
struct jtimespec jtmtil(UI ns){ //returns the time ns ns in the future
 struct jtimespec r=jmtclk();
 r.tv_sec+=ns/1000000000ull;r.tv_nsec+=ns%1000000000ull;
 if(r.tv_nsec>=1000000000ll){r.tv_sec++;r.tv_nsec-=1000000000ll;}
 R r;}
I jtmdif(struct jtimespec w){ //returns the time in ns between the current time and w, or -1 if it is not in the future
 struct jtimespec t=jmtclk();
 if(t.tv_sec>w.tv_sec||t.tv_sec==w.tv_sec&&t.tv_nsec>=w.tv_nsec)R -1;
 R (w.tv_sec-t.tv_sec)*1000000000ull+w.tv_nsec-t.tv_nsec;}

#if PYXES
// implement jfutex_wait _waitn _wake1 _wakea; synchronisation primitives
#if defined(__APPLE__)
void jfutex_wake1(UI4 *p){__ulock_wake(UL_COMPARE_AND_WAIT|ULF_NO_ERRNO,p,0);}
void jfutex_wakea(UI4 *p){__ulock_wake(UL_COMPARE_AND_WAIT|ULF_NO_ERRNO|ULF_WAKE_ALL,p,0);}
C jfutex_wait(UI4 *p,UI4 v){
 I r=__ulock_wait(UL_COMPARE_AND_WAIT|ULF_NO_ERRNO,p,v,0);
 if(r>=0)R 0;
 if(r==-EINTR||r==-EFAULT)R 0; //EFAULT means the address needed to be paged in, not that it wasn't mapped?
 R EVFACE;} //should never happen?
#if defined(__arm64__)||defined(__aarch64__)
// wait2 takes an ns timeout, but it's only available from macos 11 onward; coincidentally, arm macs only support macos 11+
// so we can count on having this
I jfutex_waitn(UI4 *p,UI4 v,UI ns){
 I r=__ulock_wait2(UL_COMPARE_AND_WAIT|ULF_NO_ERRNO,p,v,ns,0);
 if(r>=0)R 0;
 if(r==-ETIMEDOUT)R -1;
 if(r==-EINTR||r==-EFAULT)R 0; //manifest EINTR as a spurious wake, triggering a look at adbreak, instead of returning EVATTN.  The caller will have to look at adbreak anyway, since the signal might have come right after waking up, also because attention interrupt might be disabled anyway...
                               //(also, there is a race, in the case when ^C is pressed right _before_ going to sleep.  Fixable; annoying; but the window is small and the user can just press ^C again so low-priority)
 if(r==-ENOMEM)R EVWSFULL;//lol
 R EVFACE;}
#else
// but for the x86 case, we keep compatibility with older macos.  Revisit in the future
// deal with >32 bits; 2^32us is just a little over an hour; just too close for comfort
I jfutex_waitn(UI4 *p,UI4 v,UI ns){I r;
 UI us=ns/1000;
 while(us>0xfffffff){
  r=__ulock_wait(UL_COMPARE_AND_WAIT|ULF_NO_ERRNO,p,v,0xffffffff);
  if(r!=-ETIMEDOUT)goto out;
  us-=0xffffffff;}
 r=__ulock_wait(UL_COMPARE_AND_WAIT|ULF_NO_ERRNO,p,v,us);
out:
 if(r>=0)R 0;
 if(r==-ETIMEDOUT)R -1;
 if(r==-EINTR||r==-EFAULT)R 0;
 if(r==-ENOMEM)R EVWSFULL;
 R EVFACE;}
#endif
#elif defined(__linux__)
#include<unistd.h>
#if defined(__NR_futex) && !defined(SYS_futex) //android seemingly defines the former, but not the latter
#define SYS_futex __NR_futex
#endif
void jfutex_wake1(UI4 *p){syscall(SYS_futex,p,FUTEX_WAKE_PRIVATE,1);} //_PRIVATE means the address is not shared between multiple processes
void jfutex_waken(UI4 *p,UI4 n){syscall(SYS_futex,p,FUTEX_WAKE_PRIVATE,n);}
void jfutex_wakea(UI4 *p){syscall(SYS_futex,p,FUTEX_WAKE_PRIVATE,0xffffffff);}
C jfutex_wait(UI4 *p,UI4 v){
 struct timespec *pts = 0;
 int r=syscall(SYS_futex,p,FUTEX_WAIT_PRIVATE,v,pts);
 if(r>=0)R 0;
 if(errno==EAGAIN||errno==EINTR)R 0;
 R EVFACE;}
I jfutex_waitn(UI4 *p,UI4 v,UI ns){
 struct timespec ts={.tv_sec=ns/1000000000, .tv_nsec=ns%1000000000}; //ts is relative (except for the cases in which it's absolute, but this is not one such)
 int r=syscall(SYS_futex,p,FUTEX_WAIT_PRIVATE,v,&ts);
 if(r>=0)R 0;
 if(errno==ETIMEDOUT)R -1;
 if(errno==EAGAIN||errno==EINTR)R 0;
 R EVFACE;}
#elif defined(_WIN32)
// defined in cd.c to avoid name collisions between j.h and windows.h
#elif defined(__OpenBSD__) && 0
// see comment in mt.h
void jfutex_wake1(UI4 *p){futex(p,FUTEX_WAKE,1,0,0);}
void jfutex_waken(UI4 *p,UI4 n){futex(p,FUTEX_WAKE,n,0,0);}
void jfutex_wakea(UI4 *p){futex(p,FUTEX_WAKE,0x7fffffff,0,0);}
C jfutex_wait(UI4 *p,UI4 v){
 int r=futex(p,FUTEX_WAIT,v,0,0);
 if(r>=0)R 0;
 if(errno==EAGAIN||errno==EINTR||errno==ECANCELED)R 0;
 R EVFACE;}
I jfutex_waitn(UI4 *p,UI4 v,UI ns){
 struct timespec ts={.tv_sec=ns/1000000000, .tv_nsec=ns%1000000000};
 int r=futex(p,FUTEX_WAIT,v,&ts,0);
 if(r>=0)R 0;
 if(errno==ETIMEDOUT)R -1;
 if(errno==EAGAIN||errno==EINTR||errno==ECANCELED)R 0;
 R EVFACE;}
#endif

#ifndef __linux__ //no native waken on non-linux
void jfutex_waken(UI4 *p,UI4 n){jfutex_wakea(p);} //scaf/TUNE: should DO(n,jfutex_wake1(p)) depending on n and the #threads waiting on p
#endif

//values for mutex->v
//todo consider storing owner in the high bits of v
enum{
 FREE=0,  //a free mutex
 LOCK=1,  //a mutex which is held, and which no one is waiting on; uncontended mutexes will just move between FREE and LOCK
 WAIT=2}; //a mutex which is held, and which _may_ have threads waiting on it

// macos: can't use UL_UNFAIR_LOCK, as our ids are task ids, not thread ids/mach ports/whatever

// there is a flaw.  If t0 holds lock, t1 attempts to acquire it; when it eventually does, it will leave WAIT in v instead of LOCK, 

void jtpthread_mutex_init(jtpthread_mutex_t *m,B recursive){*m=(jtpthread_mutex_t){.recursive=recursive};}

C jtpthread_mutex_lock(J jt,jtpthread_mutex_t *m,I self){ //lock m
 if(uncommon(m->owner==self)){if(unlikely(!m->recursive))R EVCONCURRENCY; m->ct++;R 0;} //handle deadlock and recursive cases
 if(likely(casa(&m->v,&(UI4){FREE},LOCK)))goto success; //fast and common path: attempt to install LOCK in place of FREE; if so, we have acquired the lock
// obsolete  if(e!=WAIT)e=xchga(&m->v,WAIT); //nudge m->v towards the WAIT state.  In the unlikely event that e==WAIT&&m->v!=WAIT, fine; we'll catch it in futex_wait, since we won't get put to sleep if m->v!=WAIT
 // The lock was in use.  We will (almost always) have to wait for it
 I r;
 sta(&jt->futexwt,&m->v); //ensure other threads know how to wake us up for systemlock
// obsolete  while(e!=FREE){ //exit when e==FREE; i.e., _we_ successfully installed WAIT in place of FREE
 // It is always safe to move the state of a lock to WAIT using xchg.  There are 2 cases:
 // (1) if the previous state was FREE, we now own the lock after the xchg...
 while(xchga(&m->v,WAIT)!=FREE){ //exit when _we_ successfully installed WAIT in place of FREE
  // ... (2) the lock had an owner.  By moving state to WAIT, we guaranteed that the owner will wake us on freeing the lock
  // Before waiting, handle system events if present
  S attn=lda((S*)&JT(jt,adbreakr)[0]);
  if(unlikely(attn>>8))jtsystemlockaccept(jt,LOCKPRISYM+LOCKPRIPATH+LOCKPRIDEBUG); //check for systemlock
  if(unlikely(attn&0xff)){r=attn&0xff;goto fail;} //or attention interrupt
  // Now wait for a change.  The futex_wait is atomic, and will wait only if the state is WAIT.  In that case,
  // the holder is guaranteed to perform a wake after freeing the lock.  If the state is not WAIT, something has happened already and we inspect it forthwith
#if __linux__
  I i=jfutex_waitn(&m->v,WAIT,(UI)-1);
  //kernel bug? futex wait doesn't get interrupted by signals on linux if timeout is null
#else
  I i=jfutex_wait(&m->v,WAIT);
#endif
  if(unlikely(i>0)){r=i;goto fail;} //handle error (unaligned unmapped interrupted...)
// obsolete   e=xchga(&m->v,WAIT);
 }
 // come out of loop when we have the lock
 //note that we must install WAIT in m->v even in the case when no one is actually waiting, because we can't know if somebody else is waiting
 //ulrich drepper 'futexes are tricky' explains the issue with storing a waiter count in the value
 //a couple of alternatives suggest themselves: store up to k waiters (FREE/LOCK/WAIT is really 0/1/n; we could do eg 0/1/2/3/n); store the waiter count somehow outside of the value
success:sta(&jt->futexwt,0);m->ct+=m->recursive;m->owner=self;R 0;
fail:sta(&jt->futexwt,0);R r;}

I jtpthread_mutex_timedlock(J jt,jtpthread_mutex_t *m,UI ns,I self){ //lock m, with a timeout of ns ns.  Largely the same as lock
 if(uncommon(m->owner==self)){if(unlikely(!m->recursive))R EVCONCURRENCY; m->ct++;R 0;}
 UI4 e=FREE;if(casa(&m->v,&e,LOCK))goto success;
 struct jtimespec tgt=jtmtil(ns);
 if(e!=WAIT){e=xchga(&m->v,WAIT);if(e==FREE)goto success;}
 I r;
 sta(&jt->futexwt,&m->v);
 while(1){
  S attn=lda((S*)&JT(jt,adbreakr)[0]);
  if(attn>>8)jtsystemlockaccept(jt,LOCKPRISYM+LOCKPRIPATH+LOCKPRIDEBUG);
  if(attn&0xff){r=attn&0xff;goto fail;}
  I i=jfutex_waitn(&m->v,WAIT,ns);
  if(unlikely(i>0)){r=i;goto fail;}
  e=xchga(&m->v,WAIT);
  if(e==FREE)goto success;
  if(i==-1){r=-1;goto fail;} //if the kernel says we timed out, trust it rather than doing another syscall to check the time
  if(-1ull==(ns=jtmdif(tgt))){r=-1;goto fail;}} //update delta, abort if timed out
success:sta(&jt->futexwt,0);m->ct+=m->recursive;m->owner=self; R 0;
fail:sta(&jt->futexwt,0);R r;}


I jtpthread_mutex_trylock(jtpthread_mutex_t *m,I self){ //attempt to acquire m
 if(uncommon(m->recursive)&&m->owner){if(m->owner!=self)R -1; m->ct++;R 0;} //recursive case.  If m->owner is set the first time we read it, and clear the second time, fine; we still observed the mutex to be locked, meaning it was locked concurrently with the trylock, so it is fine to declare it locked
 if(unlikely(m->owner==self))R EVCONCURRENCY; //error to trylock a lock you hold.  I deliberated for a long time on this.  One way to see it is that trylock should be the same as timedlock(ns=0), and timedlock should return an error if you already hold the lock.  Another is that this is a concurrency primitive, giving information about concurrent events, but lock and trylock from the same thread are not concurrent events, so no useful information should be created
 if(casa(&m->v,&(UI4){FREE},LOCK)){m->ct+=m->recursive;m->owner=self;R 0;}   R -1;} //fastpath: attempt to acquire the lock

C jtpthread_mutex_unlock(jtpthread_mutex_t *m,I self){ //release m
 if(unlikely(m->owner!=self))R EVCONCURRENCY; //error to release a lock you don't hold
 if(uncommon(m->recursive)){if(--m->ct)R 0;} //need to be released more times on this thread, so nothing more to do
 m->owner=0;
// obsolete  if(!casa(&m->v,&(UI4){LOCK},FREE)){sta(&m->v,FREE);jfutex_wake1(&m->v);} //cas is fastpath LOCK->FREE.  If it fails, the state was WAIT, so we need to wake a waiter
 if(unlikely(xchga(&m->v,FREE)==WAIT))jfutex_wake1(&m->v);  // move to FREE state; if state was WAIT, wake up a waiter
 //below is what drepper does; I think the above is always faster, but it should definitely be faster without xadd
 //agner sez lock xadd has one cycle better latency vs lock cmpxchg on intel ... ??
 //(probably that's only in the uncontended case)
 //if(adda(&m->v,-1)){sta(&m->v,FREE);jfutex_wake1(&m->v);}
 R 0;}
#endif //PYXES
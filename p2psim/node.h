#ifndef __NODE_H
#define __NODE_H

#include "threaded.h"
#include <lib9.h>
#include <thread.h>
#include <map>
#include <string>
#include "p2psim.h"
#include "packet.h"
#include "rpchandle.h"
using namespace std;

class Protocol;

class Node : public Threaded {
public:
  Node(IPAddress);
  virtual ~Node();

  IPAddress ip() { return _ip; }
  void register_proto(Protocol *);
  Protocol *getproto(string p) { return _protmap[p]; }
  bool _doRPC(IPAddress, void (*fn)(void *), void *args);


  // This doRPC is not tied to Protocols, but it does require you
  // to specify the target object on the remote node.
  template<class BT, class AT, class RT>
  bool Node::doRPC(IPAddress dst, BT *target, void (BT::*fn)(AT*, RT*),
      AT *args, RT *ret)
  {
    Thunk<BT, AT, RT> *t = _makeThunk(dst, target, fn, args, ret);
    bool ok = _doRPC(dst, Thunk<BT, AT, RT>::thunk, (void *) t);
    delete t;
    return ok;
  }
  
  // This doRPC is not tied to Protocols, but it does require you
  // to specify the target object on the remote node.
  // It's asynchronous
  template<class BT, class AT, class RT>
  RPCHandle* asyncRPC(IPAddress dst, BT *target, void (BT::*fn)(AT*, RT*),
      AT *args, RT *ret)
  {
    Thunk<BT, AT, RT> *t = _makeThunk(dst, target, fn, args, ret);
    RPCHandle *rpch = _doRPC_send(dst, Thunk<BT, AT, RT>::thunk, (void *) t);
    return rpch;
  }


  Channel *pktchan() { return _pktchan; }
  void crash () { _alive = false; }
  bool alive () { return _alive; }
  void set_alive() { _alive = true;}

  // Parent of Thunk so that RPCHandle can call delete on a Thunk without the
  // template parameters
  class Thing {
  };

private:

  // Thunk inherits from Thing so that we can delete it in RPCHandle without
  // needing the parameters.  Note, however, that Thunk's constructor will NOT
  // be called.
  template<class BT, class AT, class RT>
  class Thunk : public Thing {
  public:
    BT *_target;
    void (BT::*_fn)(AT *, RT *);
    AT *_args;
    RT *_ret;
    static void thunk(void *xa) {
      Thunk *t = (Thunk *) xa;
      (t->_target->*(t->_fn))(t->_args, t->_ret);
    }
  };

  // implements _doRPC
  RPCHandle* _doRPC_send(IPAddress, void (*)(void *), void *);
  bool _doRPC_receive(RPCHandle*);


  // creates a Thunk object with the necessary croft for an RPC
  template<class BT, class AT, class RT>
  Node::Thunk<BT, AT, RT> *
  Node::_makeThunk(IPAddress dst, BT *target, void (BT::*fn)(AT*, RT*),
      AT *args, RT *ret)
  {
    // target is probably the result of a dynamic_cast<BT*>...
    assert(target);

    Thunk<BT, AT, RT>  *t = new Thunk<BT, AT, RT>;
    t->_target = target;
    t->_fn = fn;
    t->_args = args;
    t->_ret = ret;

    return t;
  }



  virtual void run();
  static void Receive(void*);

  IPAddress _ip;        // my ip address
  bool _alive;
  Channel *_pktchan;    // for packets

  typedef map<string, Protocol*> PM;
  typedef PM::const_iterator PMCI;
  PM _protmap;
};

#endif // __NODE_H

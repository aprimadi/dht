#ifndef _ROUTE_SECCHORD_H_
#define _ROUTE_SECCHORD_H_

// XXX route.h should only contain the interface for the route superclasses
#include "route.h"
#include "skiplist.h"

/**
 * A secure variant of Chord routing, utilizing successor lists at each hop.
 * The route returned by path contains a subset of the actual nodes contacted.
 *
 * This is only a moderately good fit for the route_iterator interface.  It
 * has some issues with when upcalls should be sent and really wants to be
 * able to return successor lists to the caller at each hop.
 */
class route_secchord : public route_iterator {
  struct node {
    chordID n_;
    chord_node cn_;
    u_int count_;
    sklist_entry<node> sortlink_;

    node (const chord_node &n) : n_ (n.x), cn_ (n), count_ (0) {};
    node (const chordID &n, const net_address &r) :
      n_ (n), count_ (0) { cn_.x = n; cn_.r = r; };
  };
  skiplist<node, chordID, &node::n_, &node::sortlink_> nexthops_;
  // An indicator used to ensure that any upcall is delivered to the
  // actual successor of a key.
  bool lasthop_;

  // Has some node returned a pair of nodes bracketing the key yet?
  u_int bracketed_key_;
  // Do we have the required number of successors?
  bool sufficient_successors ();
  void clear_nexthops ();

  u_int outstanding_; // number of outstanding RPCs for this hop.
  void next_hop_cb (ptr<bool> del, chord_node dst,
		    chord_nodelistres *res, clnt_stat err);
  void send_hop_cb (bool done);
  
 public:
  route_secchord (ptr<vnode> vi, chordID xi);
  route_secchord (ptr<vnode> vi, chordID xi,
		  rpc_program uc_prog,
		  int uc_procno,
		  ptr<void> uc_args);
  ~route_secchord ();
  void print ();
  void send (chordID guess);
  void send (bool ucs);
  void first_hop (cbhop_t cb, bool ucs = false);
  void first_hop (cbhop_t cb, chordID guess);
  void next_hop ();
  chordID pop_back ();
};

class secchord_route_factory : public route_factory {
 public:
  secchord_route_factory (ptr<vnode> vi) : route_factory (vi) {};
  secchord_route_factory () {};

  ptr<route_iterator> produce_iterator (chordID xi);
  ptr<route_iterator> produce_iterator (chordID xi,
					rpc_program uc_prog,
					int uc_procno,
					ptr<void> uc_args);
  route_iterator *produce_iterator_ptr (chordID xi);
  route_iterator *produce_iterator_ptr (chordID xi,
					rpc_program uc_prog,
					int uc_procno,
					ptr<void> uc_args);
};

#endif /* !_ROUTE_SECCHORD_H_ */
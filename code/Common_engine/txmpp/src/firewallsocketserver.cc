/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "firewallsocketserver.h"

#include <cassert>
#include <algorithm>

#include "asyncsocket.h"
#include "logging.h"

namespace txmpp {

class FirewallSocket : public AsyncSocketAdapter {
 public:
  FirewallSocket(FirewallSocketServer* server, AsyncSocket* socket, int type)
    : AsyncSocketAdapter(socket), server_(server), type_(type) {
  }

  virtual int Connect(const SocketAddress& addr) {
    if (type_ == SOCK_STREAM) {
      if (!server_->Check(FP_TCP, GetLocalAddress(), addr)) {
        LOG(LS_VERBOSE) << "FirewallSocket outbound TCP connection from "
                        << GetLocalAddress().ToString() << " to "
                        << addr.ToString() << " denied";
        // TODO(juberti): Handle this asynchronously.
        SetError(EHOSTUNREACH);
        return SOCKET_ERROR;
      }
    }
    return AsyncSocketAdapter::Connect(addr);
  }
  virtual int Send(const void* pv, size_t cb) {
    return SendTo(pv, cb, GetRemoteAddress());
  }
  virtual int SendTo(const void* pv, size_t cb, const SocketAddress& addr) {
    if (type_ == SOCK_DGRAM) {
      if (!server_->Check(FP_UDP, GetLocalAddress(), addr)) {
        LOG(LS_VERBOSE) << "FirewallSocket outbound UDP packet from "
                        << GetLocalAddress().ToString() << " to "
                        << addr.ToString() << " dropped";
        return static_cast<int>(cb);
      }
    }
    return AsyncSocketAdapter::SendTo(pv, cb, addr);
  }
  virtual int Recv(void* pv, size_t cb) {
    SocketAddress addr;
    return RecvFrom(pv, cb, &addr);
  }
  virtual int RecvFrom(void* pv, size_t cb, SocketAddress* paddr) {
    if (type_ == SOCK_DGRAM) {
      while (true) {
        int res = AsyncSocketAdapter::RecvFrom(pv, cb, paddr);
        if (res <= 0)
          return res;
        if (server_->Check(FP_UDP, *paddr, GetLocalAddress()))
          return res;
        LOG(LS_VERBOSE) << "FirewallSocket inbound UDP packet from "
                        << paddr->ToString() << " to "
                        << GetLocalAddress().ToString() << " dropped";
      }
    }
    return AsyncSocketAdapter::RecvFrom(pv, cb, paddr);
  }

  virtual int Listen(int backlog) {
    if (!server_->tcp_listen_enabled()) {
      LOG(LS_VERBOSE) << "FirewallSocket listen attempt denied";
      return -1;
    }

    return AsyncSocketAdapter::Listen(backlog);
  }
  virtual AsyncSocket* Accept(SocketAddress* paddr) {
    SocketAddress addr;
    while (AsyncSocket* sock = AsyncSocketAdapter::Accept(&addr)) {
      if (server_->Check(FP_TCP, addr, GetLocalAddress())) {
        if (paddr)
          *paddr = addr;
        return sock;
      }
      sock->Close();
      delete sock;
      LOG(LS_VERBOSE) << "FirewallSocket inbound TCP connection from "
                      << addr.ToString() << " to "
                      << GetLocalAddress().ToString() << " denied";
    }
    return 0;
  }

 private:
  FirewallSocketServer* server_;
  int type_;
};

FirewallSocketServer::FirewallSocketServer(SocketServer* server,
                                           FirewallManager* manager,
                                           bool should_delete_server)
    : server_(server), manager_(manager),
      should_delete_server_(should_delete_server),
      udp_sockets_enabled_(true), tcp_sockets_enabled_(true),
      tcp_listen_enabled_(true) {
  if (manager_)
    manager_->AddServer(this);
}

FirewallSocketServer::~FirewallSocketServer() {
  if (manager_)
    manager_->RemoveServer(this);

  if (server_ && should_delete_server_) {
    delete server_;
    server_ = NULL;
  }
}

void FirewallSocketServer::AddRule(bool allow, FirewallProtocol p,
                                   FirewallDirection d,
                                   const SocketAddress& addr) {
  SocketAddress src, dst;
  if (d == FD_IN) {
    dst = addr;
  } else {
    src = addr;
  }
  AddRule(allow, p, src, dst);
}


void FirewallSocketServer::AddRule(bool allow, FirewallProtocol p,
                                   const SocketAddress& src,
                                   const SocketAddress& dst) {
  Rule r;
  r.allow = allow;
  r.p = p;
  r.src = src;
  r.dst = dst;
  CritScope scope(&crit_);
  rules_.push_back(r);
}

void FirewallSocketServer::ClearRules() {
  CritScope scope(&crit_);
  rules_.clear();
}

bool FirewallSocketServer::Check(FirewallProtocol p,
                                 const SocketAddress& src,
                                 const SocketAddress& dst) {
  CritScope scope(&crit_);
  for (size_t i = 0; i < rules_.size(); ++i) {
    const Rule& r = rules_[i];
    if ((r.p != p) && (r.p != FP_ANY))
      continue;
    if ((r.src.ip() != src.ip()) && !r.src.IsAny())
      continue;
    if ((r.src.port() != src.port()) && (r.src.port() != 0))
      continue;
    if ((r.dst.ip() != dst.ip()) && !r.dst.IsAny())
      continue;
    if ((r.dst.port() != dst.port()) && (r.dst.port() != 0))
      continue;
    return r.allow;
  }
  return true;
}

Socket* FirewallSocketServer::CreateSocket(int type) {
  return WrapSocket(server_->CreateAsyncSocket(type), type);
}

AsyncSocket* FirewallSocketServer::CreateAsyncSocket(int type) {
  return WrapSocket(server_->CreateAsyncSocket(type), type);
}

AsyncSocket* FirewallSocketServer::WrapSocket(AsyncSocket* sock, int type) {
  if (!sock ||
      (type == SOCK_STREAM && !tcp_sockets_enabled_) ||
      (type == SOCK_DGRAM && !udp_sockets_enabled_)) {
    LOG(LS_VERBOSE) << "FirewallSocketServer socket creation denied";
    return NULL;
  }
  return new FirewallSocket(this, sock, type);
}

FirewallManager::FirewallManager() {
}

FirewallManager::~FirewallManager() {
  assert(servers_.empty());
}

void FirewallManager::AddServer(FirewallSocketServer* server) {
  CritScope scope(&crit_);
  servers_.push_back(server);
}

void FirewallManager::RemoveServer(FirewallSocketServer* server) {
  CritScope scope(&crit_);
  servers_.erase(std::remove(servers_.begin(), servers_.end(), server),
                 servers_.end());
}

void FirewallManager::AddRule(bool allow, FirewallProtocol p,
                              FirewallDirection d, const SocketAddress& addr) {
  CritScope scope(&crit_);
  for (std::vector<FirewallSocketServer*>::const_iterator it =
      servers_.begin(); it != servers_.end(); ++it) {
    (*it)->AddRule(allow, p, d, addr);
  }
}

void FirewallManager::ClearRules() {
  CritScope scope(&crit_);
  for (std::vector<FirewallSocketServer*>::const_iterator it =
      servers_.begin(); it != servers_.end(); ++it) {
    (*it)->ClearRules();
  }
}

}  // namespace txmpp

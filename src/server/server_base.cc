// Copyright (c) 2013, Cloudera, inc.

#include "server/server_base.h"

#include <gflags/gflags.h>
#include <vector>

#include "rpc/messenger.h"
#include "server/default-path-handlers.h"
#include "server/rpc_server.h"
#include "server/webserver.h"
#include "util/net/sockaddr.h"

DEFINE_int32(num_reactor_threads, 4, "Number of libev reactor threads to start."
             " (Advanced option).");

using std::vector;

namespace kudu {
namespace server {

ServerBase::ServerBase(const RpcServerOptions& rpc_opts,
                       const WebserverOptions& web_opts)
  : rpc_server_(new RpcServer(rpc_opts)),
    web_server_(new Webserver(web_opts)) {
}

ServerBase::~ServerBase() {
  Shutdown();
}

Sockaddr ServerBase::first_rpc_address() const {
  vector<Sockaddr> addrs;
  rpc_server_->GetBoundAddresses(&addrs);
  CHECK(!addrs.empty()) << "Not bound";
  return addrs[0];
}

Sockaddr ServerBase::first_http_address() const {
  vector<Sockaddr> addrs;
  web_server_->GetBoundAddresses(&addrs);
  CHECK(!addrs.empty()) << "Not bound";
  return addrs[0];
}

Status ServerBase::Init() {
  // Create the Messenger.
  rpc::MessengerBuilder builder("TODO: add a ToString for ServerBase");

  builder.set_num_reactors(FLAGS_num_reactor_threads);
  RETURN_NOT_OK(builder.Build(&messenger_));

  RETURN_NOT_OK(rpc_server_->Init());
  return Status::OK();
}

Status ServerBase::Start(gscoped_ptr<rpc::ServiceIf> rpc_impl) {
  RETURN_NOT_OK(rpc_server_->Start(messenger_, rpc_impl.Pass()));

  AddDefaultPathHandlers(web_server_.get());
  RETURN_NOT_OK(web_server_->Start());
  return Status::OK();
}

Status ServerBase::Shutdown() {
  if (messenger_) {
    messenger_->Shutdown();
    messenger_.reset();
  }
  web_server_->Stop();
  rpc_server_->Shutdown();
  return Status::OK();
}

} // namespace server
} // namespace kudu

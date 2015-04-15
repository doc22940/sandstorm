// Sandstorm - Personal Cloud Sandbox
// Copyright (c) 2015 Sandstorm Development Group, Inc. and contributors
// All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SANDSTORM_BACKEND_H_
#define SANDSTORM_BACKEND_H_

#include <sandstorm/backend.capnp.h>
#include <map>
#include <kj/async-io.h>
#include <capnp/rpc-twoparty.h>
#include <kj/one-of.h>

namespace sandstorm {

class BackendImpl: public Backend::Server, private kj::TaskSet::ErrorHandler {
public:
  BackendImpl(kj::LowLevelAsyncIoProvider& ioProvider, kj::Network& network);

protected:
  kj::Promise<void> startGrain(StartGrainContext context) override;
  kj::Promise<void> getGrain(GetGrainContext context) override;
  kj::Promise<void> deleteGrain(DeleteGrainContext context) override;
  kj::Promise<void> installPackage(InstallPackageContext context) override;

private:
  kj::LowLevelAsyncIoProvider& ioProvider;
  kj::Network& network;
  kj::TaskSet tasks;

  class RunningGrain {
  public:
    RunningGrain(BackendImpl& backend, kj::String grainId, kj::Own<kj::AsyncIoStream> stream);
    ~RunningGrain() noexcept(false);

    inline kj::StringPtr getGrainId() { return grainId; }
    inline kj::Promise<void> onDisconnect() { return client.onDisconnect(); }

    inline Supervisor::Client getSupervisor() {
      return client.bootstrap().castAs<Supervisor>();
    }

  private:
    BackendImpl& backend;
    kj::String grainId;
    kj::Own<kj::AsyncInputStream> stdout;
    kj::Own<kj::AsyncIoStream> stream;
    capnp::TwoPartyClient client;
  };

  struct StartingGrain {
    kj::String grainId;
    kj::ForkedPromise<Supervisor::Client> promise;
  };

  std::map<kj::StringPtr, StartingGrain> supervisors;

  kj::Promise<Supervisor::Client> bootGrain(kj::StringPtr grainId, kj::StringPtr packageId,
      spk::Manifest::Command::Reader command, bool isNew, bool devMode, bool isRetry);

  static kj::Promise<void> ignoreAll(kj::AsyncInputStream& input);

  void taskFailed(kj::Exception&& exception) override;
};

}  // namespace sandstorm

#endif // SANDSTORM_BACKEND_H_

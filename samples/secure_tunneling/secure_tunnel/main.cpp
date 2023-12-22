/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <aws/crt/Api.h>
#include <aws/iotdevicecommon/IotDevice.h>
#include <aws/iotsecuretunneling/SecureTunnel.h>

#include <future>
#include <iostream>

using namespace Aws::Crt;
using namespace Aws::Iotsecuretunneling;

void test()
{
    std::promise<bool> clientStopped;
    std::promise<bool> clientConnected;

    const auto setConnected = [&](bool value){
        try {
            clientConnected.set_value(value);
        }
        catch(const std::exception& e) {
            std::cerr << "Set Started value:" << value << " " << e.what() << std::endl;
        }
    };

    auto builder = SecureTunnelBuilder(
        aws_default_allocator(), "token", AWS_SECURE_TUNNELING_DESTINATION_MODE, "data.region.fake");
    // OR 
    // auto bootstrap = Aws::Crt::ApiHandle::GetOrCreateStaticDefaultClientBootstrap();
    // auto builder = SecureTunnelBuilder{aws_default_allocator(), *bootstrap, {},
    //                 "token", AWS_SECURE_TUNNELING_DESTINATION_MODE, "data.region.fake"};

    builder.WithOnConnectionStarted([&](SecureTunnel *,
                                        int errorCode,
                                        const ConnectionStartedEventData &) {
        std::cout << "Connection Started:" << ErrorDebugString(errorCode) << std::endl;
        setConnected(true);
    });
    builder.WithOnConnectionFailure([&](SecureTunnel *, int errorCode) {
        std::cout << "Connection Failure:" << ErrorDebugString(errorCode) << std::endl;
        setConnected(false);
    });
    builder.WithOnConnectionShutdown([&]() {
        std::cout << "Connection Shutdown" << std::endl;
    });
    builder.WithOnStopped([&](SecureTunnel *secureTunnel) {
        std::cout << "Secure Tunnel has entered Stopped State" << std::endl;
        clientStopped.set_value(true);
    });

    // Create Secure Tunnel using the options set with the builder
    auto secureTunnel = builder.Build();

    auto err = secureTunnel->Start();
    if (err)
    {
        std::cerr << "Start with: " << err << " " << ErrorDebugString(LastError()) << std::endl;
        exit(-1);
    }

    bool isConnected = clientConnected.get_future().get();
    std::cout << "isConnected: " << isConnected << std::endl;

    // Set the Secure Tunnel Client to desire a stopped state
    if (secureTunnel->Stop() == AWS_OP_ERR)
    {
        std::cerr << "Secure Tunnel Stop call failed: " << ErrorDebugString(LastError());
    }

    // The Secure Tunnel Client at this point will report they are stopped and can be safely removed.
    if (clientStopped.get_future().get())
    {
        secureTunnel = nullptr;
    }
}

int main(int argc, char *argv[])
{
    ApiHandle apiHandle;

    for(std::size_t i = 0, iterations = 10000; i < iterations; ++i)
    {
        std::cout << "Iteration: " << i << "/" << iterations << '\n';
        test();
    }

    return 0;
}

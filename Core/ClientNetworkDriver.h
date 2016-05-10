#pragma once
#include "IBaseNetworkDriver.h"
#include <memory>
#include "Mouse.h"

namespace SL {
	namespace Remote_Access_Library {
		namespace Utilities {
			class Point;
		}
		namespace Network {
			class IClientDriver;
			class ClientNetworkDriverImpl;

			class ClientNetworkDriver {
				std::unique_ptr<ClientNetworkDriverImpl> _ClientNetworkDriverImpl;

			public:
				ClientNetworkDriver(IClientDriver* r, const char* dst_host, const char* dst_port);
				virtual ~ClientNetworkDriver();

				//after creating ServerNetworkDriver, Start() must be called to start the network processessing
				void Start();
				//Before calling Stop, you must ensure that any external references to shared_ptr<ISocket> have been released
				void Stop();

				void SendMouse(const Input::MouseEvent& m);
				bool ConnectedToSelf() const;
			};
		}
	}
}

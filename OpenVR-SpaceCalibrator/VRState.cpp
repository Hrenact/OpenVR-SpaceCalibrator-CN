#include "stdafx.h"
#include "VRState.h"

VRState VRState::Load()
{
	VRState state;
	auto& trackingSystems = state.trackingSystems;

	char buffer[vr::k_unMaxPropertyStringSize];

	for (uint32_t id = 0; id < vr::k_unMaxTrackedDeviceCount; ++id)
	{
		vr::ETrackedPropertyError err = vr::TrackedProp_Success;
		auto deviceClass = vr::VRSystem()->GetTrackedDeviceClass(id);
		if (deviceClass == vr::TrackedDeviceClass_Invalid)
			continue;

		if (deviceClass != vr::TrackedDeviceClass_TrackingReference)
		{
			vr::VRSystem()->GetStringTrackedDeviceProperty(id, vr::Prop_TrackingSystemName_String, buffer, vr::k_unMaxPropertyStringSize, &err);

			if (err == vr::TrackedProp_Success)
			{
				std::string system(buffer);

				// Check if the current HMD is a Pimax crystal
				if (deviceClass == vr::TrackedDeviceClass_HMD && system == "aapvr") {
					// HMD is a Pimax HMD
					vr::HmdMatrix34_t eyeToHeadLeft = vr::VRSystem()->GetEyeToHeadTransform(vr::Eye_Left);
					// Crystal's projection matrix is constant 0s or 1s except for [0][3], which stores the IPD offset from the nose
					bool isCrystalHmd =
						eyeToHeadLeft.m[0][0] == 1 && eyeToHeadLeft.m[0][1] == 0 && eyeToHeadLeft.m[0][2] == 0 &&                     // IPD
						eyeToHeadLeft.m[1][0] == 0 && eyeToHeadLeft.m[1][1] == 1 && eyeToHeadLeft.m[1][2] == 0 && eyeToHeadLeft.m[1][3] == 0 &&
						eyeToHeadLeft.m[2][0] == 0 && eyeToHeadLeft.m[2][1] == 0 && eyeToHeadLeft.m[2][2] == 1 && eyeToHeadLeft.m[2][3] == 0;

					if (isCrystalHmd) {
						// Move it outside the aapvr system ; we treat aapvr as if it were lighthouse
						system = "Pimax Crystal HMD";
					}
				} else if (deviceClass == vr::TrackedDeviceClass_Controller && system == "oculus") {
					vr::VRSystem()->GetStringTrackedDeviceProperty(id, vr::Prop_RenderModelName_String, buffer, vr::k_unMaxPropertyStringSize, &err);
					std::string renderModel(buffer);
					vr::VRSystem()->GetStringTrackedDeviceProperty(id, vr::Prop_ConnectedWirelessDongle_String, buffer, vr::k_unMaxPropertyStringSize, &err);
					std::string connectedWirelessDongle(buffer);

					// Check if the controller claims its an oculus controller but also pimax
					if (renderModel.find("{aapvr}") != std::string::npos &&
						renderModel.find("crystal") != std::string::npos &&
						connectedWirelessDongle.find("lighthouse") != std::string::npos) {
						system = "Pimax Crystal Controllers";
					}
				}

				auto existing = std::find(trackingSystems.begin(), trackingSystems.end(), system);
				if (existing != trackingSystems.end())
				{
					if (deviceClass == vr::TrackedDeviceClass_HMD)
					{
						trackingSystems.erase(existing);
						trackingSystems.insert(trackingSystems.begin(), system);
					}
				}
				else
				{
					trackingSystems.push_back(system);
				}

				VRDevice device;
				device.id = id;
				device.deviceClass = deviceClass;
				device.trackingSystem = system;

				vr::VRSystem()->GetStringTrackedDeviceProperty(id, vr::Prop_ModelNumber_String, buffer, vr::k_unMaxPropertyStringSize, &err);
				device.model = std::string(buffer);

				vr::VRSystem()->GetStringTrackedDeviceProperty(id, vr::Prop_SerialNumber_String, buffer, vr::k_unMaxPropertyStringSize, &err);
				device.serial = std::string(buffer);

				device.controllerRole = (vr::ETrackedControllerRole)vr::VRSystem()->GetInt32TrackedDeviceProperty(id, vr::Prop_ControllerRoleHint_Int32, &err);

				state.devices.push_back(device);
			}
			else
			{
				printf("failed to get tracking system name for id %d\n", id);
			}
		}
	}

	return state;
}

int VRState::FindDevice(const std::string& trackingSystem, const std::string& model, const std::string& serial) const {
	for (int i = 0; i < devices.size(); i++) {
		const auto& device = devices[i];
		
		if (device.trackingSystem == trackingSystem && device.model == model && device.serial == serial) return device.id;
	}

	// allow fallback excluding serial for HMD specifically
	for (int i = 0; i < devices.size(); i++) {
		const auto& device = devices[i];

		if (device.trackingSystem == trackingSystem && device.model == model && device.deviceClass == vr::TrackedDeviceClass::TrackedDeviceClass_HMD) return device.id;
	}

	return -1;
}
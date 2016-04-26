--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

parser = require("FRC_Data_Structures")

local FRC_DS_FMS = Proto("FRC_DS_FMS", "FRC Driver Station (FMS)")

local f = FRC_DS_FMS.fields
f.seqNum = ProtoField.uint16("FRC_DS_FMS.seq", "Sequence Number", base.DEC)
f.commVersion = ProtoField.uint8("FRC_DS.commVersion", "Comm Version")

f.control = ProtoField.uint8("FRC_DS_FMS.control", "Control Byte", base.HEX)
f.mode = ProtoField.uint8("FRC_DS_FMS.control.mode", "Mode")
f.enabled = ProtoField.bool("FRC_DS_FMS.control.enabled", "Enabled")
f.estop = ProtoField.bool("FRC_DS_FMS.control.estop", "Emergency Stop")
f.DSConnected = ProtoField.bool("FRC_DS_FMS.control.DSConnected", "Driver Station Connected")
f.ctrlUnknown = ProtoField.uint16("FRC_DS_FMS.control.unknown", "Unknown")

f.teamNum = ProtoField.uint16("FRC_DS_FMS.teamNum", "Team Number")
f.battery = ProtoField.string("FRC_DS_FMS.battery", "Battery")
f.unknown2 = ProtoField.bytes("FRC_DS_FMS.unknown2", "Unknown field 2")

function FRC_DS_FMS.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_DS_FMS")
	local subtree = tree:add(FRC_DS_FMS, buf())
	local info = {}

	subtree:add(f.seqNum, buf(0,2))
	subtree:add(f.commVersion, buf(2,1))

	local control = parser.parseControlBytes(buf(3, 1))

	local controlText = ""
	if (control.estop.val == 0) then
		controlText = controlText .. control.mode.text .. " " .. control.enabled.text
	else
		controlText = controlText .. control.estop.text
	end

	info["Mode"] = controlText

	local controlTree = subtree:add(f.control, control.buf )
	controlTree:append_text(" (" .. controlText .. ")")

	local items = {"raw", "mode", "enabled", "estop", "DSConnected", "FMSConnected", "unknown"}
	for i, v in ipairs(items) do
		controlTree:add(f[v], control[v].buf, control[v].val):set_text(control[v].bitstr .. " (" .. control[v].name .. ": " .. control[v].text .. ")")
	end

	subtree:add(f.teamNum, buf(4, 2))
	info["Team"] = buf(4, 2):uint()

	local batteryLevel = parser.parseBattery(buf(6, 2))
	subtree:add(f.battery, batteryLevel.buf, batteryLevel.val)
	info["Battery"] = batteryLevel.val

	subtree:add(f.unknown2, buf(8))

	local infoStr = ""
	local initial = true
	for i, v in pairs(info) do
		if (not initial) then
			infoStr = infoStr .. "  "
		end
		infoStr = infoStr .. i .. ": " .. v
		initial = false
	end

	pkt.cols.info:set("DS->FMS   " .. infoStr)

end

udp_table = DissectorTable.get("udp.port")
udp_table:add(1160, FRC_DS_FMS)

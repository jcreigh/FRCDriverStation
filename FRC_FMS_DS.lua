--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

parser = require("FRC_Data_Structures")

local FRC_FMS_DS = Proto("FRC_FMS_DS", "FRC Field Management System")

local f = FRC_FMS_DS.fields
f.seqNum = ProtoField.uint16("FRC_FMS_DS.seq", "Sequence Number", base.DEC)
f.unknown1 = ProtoField.uint8("FRC_FMS_DS.unknown1", "Unknown field 1", base.HEX)

f.control = ProtoField.uint8("FRC_FMS_DS.control", "Control Byte", base.HEX)
f.mode = ProtoField.uint8("FRC_FMS_DS.control.mode", "Mode")
f.enabled = ProtoField.bool("FRC_FMS_DS.control.enabled", "Enabled")
f.estop = ProtoField.bool("FRC_FMS_DS.control.estop", "Emergency Stop")
f.ctrlUnknown = ProtoField.uint16("FRC_FMS_DS.control.unknown", "Unknown")

f.unknown2 = ProtoField.bytes("FRC_FMS_DS.unknown2", "Unknown field 2")
f.alliance_color = ProtoField.string("FRC_FMS_DS.alliance.color", "Alliance color")
f.alliance_pos = ProtoField.uint8("FRC_FMS_DS.alliance.pos", "Alliance position")
f.unknown3 = ProtoField.bytes("FRC_FMS_DS.unknown3", "Unknown field 3")

f.date = ProtoField.bytes("FRC_FMS_DS.date", "Date")
f.date_usec = ProtoField.uint32("FRC_FMS_DS.date.microseconds", "Microseconds")
f.date_sec = ProtoField.uint8("FRC_FMS_DS.date.second", "Second")
f.date_min = ProtoField.uint8("FRC_FMS_DS.date.minute", "Minute")
f.date_hour = ProtoField.uint8("FRC_FMS_DS.date.hour", "Hour")
f.date_day = ProtoField.uint8("FRC_FMS_DS.date.day", "Day")
f.date_month = ProtoField.uint8("FRC_FMS_DS.date.month", "Month")
f.date_year = ProtoField.uint8("FRC_FMS_DS.date.year", "Year")

f.time_left = ProtoField.uint16("FRC_FMS_DS.timeLeft", "Match Time Left")


function FRC_FMS_DS.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_FMS_DS")
	local subtree = tree:add(FRC_FMS_DS, buf())
	local info = {}

	subtree:add(f.seqNum, buf(0,2))
	subtree:add(f.unknown1, buf(2,1))

	local control = parser.parseControlBytes(buf(3, 1))

	local controlText = ""
	if (control.estop.val == 0) then
		controlText = controlText .. control.mode.text .. " " .. control.enabled.text
	else
		controlText = controlText .. control.estop.text
	end

	info["Mode"] = controlText

	local controlTree = subtree:add(f.control, control.buf)
	controlTree:append_text(" (" .. controlText .. ")")

	local items = {"raw", "mode", "enabled", "estop", "unknown"}
	for i, v in ipairs(items) do
		controlTree:add(f[v], control[v].buf, control[v].val):set_text(control[v].bitstr .. " (" .. control[v].name .. ": " .. control[v].text .. ")")
	end

	subtree:add(f.unknown2, buf(4, 1))

	local alliance = parser.parseAllianceByte(buf(5, 1))
	subtree:add(f.alliance_color, alliance.buf, alliance.color)
	subtree:add(f.alliance_pos, alliance.buf, alliance.pos)
	info["Alliance"] = alliance.text

	subtree:add(f.unknown2, buf(6, 4))

	local date = parser.parseDate(buf(10))
	local dateTree = subtree:add(f.date, buf(10, 10))
	dateTree:set_text(date.text .. ": " .. date.dateText)
	dateTree:add(f.date_usec, date.usec)
	dateTree:add(f.date_sec, date.sec)
	dateTree:add(f.date_min, date.min)
	dateTree:add(f.date_hour, date.hour)
	dateTree:add(f.date_day, date.day)
	dateTree:add(f.date_month, date.month)
	dateTree:add(f.date_year, date.year)

	subtree:add(f.time_left, buf(20, 2))
	info["Time Left"] = buf(20, 2):uint()

	local infoStr = ""
	local initial = true
	for i, v in pairs(info) do
		if (not initial) then
			infoStr = infoStr .. "  "
		end
		infoStr = infoStr .. i .. ": " .. v
		initial = false
	end


	pkt.cols.info:set("FMS->DS   " .. infoStr)

end

udp_table = DissectorTable.get("udp.port")
udp_table:add(1120, FRC_FMS_DS)


--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

parser = require("FRC_Data_Structures")

local FRC_DS = Proto("FRC_DS", "FRC Driver Station")

local f = FRC_DS.fields
f.seqNum = ProtoField.uint16("FRC_DS.seq", "Sequence Number", base.DEC)

f.control = ProtoField.uint8("FRC_DS.control", "Control Byte", base.HEX)
f.mode = ProtoField.uint8("FRC_DS.control.mode", "Mode")
f.enabled = ProtoField.bool("FRC_DS.control.enabled", "Enabled")
f.estop = ProtoField.bool("FRC_DS.control.estop", "Emergency Stop")
f.FMSConnected = ProtoField.bool("FRC_DS.control.FMSConnected", "FMS Connected")
f.restartRobotCode = ProtoField.bool("FRC_DS.control.restartRobotCode", "Restart Robot Code")
f.rebootRoboRIO = ProtoField.bool("FRC_DS.control.rebootRoboRIO", "Reboot RoboRIO")
f.ctrlUnknown = ProtoField.uint16("FRC_DS.control.unknown", "Unknown")

f.alliance_color = ProtoField.string("FRC_DS.alliance.color", "Alliance color")
f.alliance_pos = ProtoField.uint8("FRC_DS.alliance.pos", "Alliance position")

f.date = ProtoField.bytes("FRC_DS.date", "Date")
f.date_usec = ProtoField.uint32("FRC_DS.date.microseconds", "Microseconds")
f.date_sec = ProtoField.uint8("FRC_DS.date.second", "Second")
f.date_min = ProtoField.uint8("FRC_DS.date.minute", "Minute")
f.date_hour = ProtoField.uint8("FRC_DS.date.hour", "Hour")
f.date_day = ProtoField.uint8("FRC_DS.date.day", "Day")
f.date_month = ProtoField.uint8("FRC_DS.date.month", "Month")
f.date_year = ProtoField.uint8("FRC_DS.date.year", "Year")

f.date_tz = ProtoField.bytes("FRC_DS.date.tz", "Timezone")
f.date_tz_size = ProtoField.uint8("FRC_DS.date.tz_size", "Timezone Size")
f.date_tz_tz = ProtoField.string("FRC_DS.date.tz", "Timezone")

f.js_count = ProtoField.uint8("FRC_DS.joystick_count", "Number of joysticks")
for js_i = 0, 5 do
	local fieldName = "FRC_DS.joystick." .. js_i

	f.js_axis_count = ProtoField.uint8(fieldName .. ".axis_count", "Number of axes")
	for i = 0, 255 do
		f["js_" .. js_i .. "_axis_" .. i] =  ProtoField.int8(fieldName .. ".axis." .. i, "Axis #" .. i)
	end

	f.js_button_count = ProtoField.uint8(fieldName .. ".button_count", "Number of buttons")
	for i = 1, 256 do
		f["js_" .. js_i .. "_button_" .. i] =  ProtoField.uint8(fieldName .. ".button." .. i, "Button #" .. i)
	end

	f.js_pov_count = ProtoField.uint8(fieldName .. ".pov_count", "Number of POVs")
	for i = 0, 255 do
		f["js_" .. js_i .. "_pov_" .. i] =  ProtoField.int16(fieldName .. ".pov." .. i, "POV #" .. i)
	end
end

function FRC_DS.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_DS")
	local subtree = tree:add(FRC_DS, buf())
	local info = {}

	subtree:add(f.seqNum, buf(0,2))
	--subtree:add(f.unknown, buf(2,1)) -- Hide, it /always/ seems to be 0x01

	local control = parser.parseControlBytes(buf(3, 2))

	local controlText = ""
	if (control.estop.val == 0) then
		controlText = controlText .. control.mode.text .. " " .. control.enabled.text
	else
		controlText = controlText .. control.estop.text
	end

	info["Mode"] = controlText

	local controlTree = subtree:add(f.control, control.buf )
	controlTree:append_text(" (" .. controlText .. ")")

	local items = {"raw", "mode", "enabled", "estop", "FMSConnected", "restartRobotCode", "rebootRoboRIO", "unknown"}
	for i, v in ipairs(items) do
		controlTree:add(f[v], control[v].buf, control[v].val):set_text(control[v].bitstr .. " (" .. control[v].name .. ": " .. control[v].text .. ")")
	end

	local alliance = parser.parseAllianceByte(buf(5, 1))
	subtree:add(f.alliance_color, alliance.buf, alliance.color)
	subtree:add(f.alliance_pos, alliance.buf, alliance.pos)

	info["Alliance"] = alliance.text

	local restData = buf(6)
	local js_i = 0
	while (restData:len() > 0) do
		local struct = parser.parseStructure(restData)
		if (struct.ID == 0x0c) then -- Joystick
			local jstree = subtree:add(struct.buf, i):set_text("Joystick #" .. js_i .. ":")

			jstree:add(f.js_axis_count, struct.axisCount)
			if (#struct.axes > 0) then
				local axisTree = jstree:add("Axes:", struct.axisBuf)
				for i, v in ipairs(struct.axes) do
					axisTree:add(f["js_" .. js_i .. "_axis_" .. (i - 1)], v)
				end
			end

			jstree:add(f.js_button_count, struct.buttonCount)
			if (#struct.buttons > 0) then
				local buttonTree = jstree:add("Buttons:", struct.buttonBuf)
				for i, v in ipairs(struct.buttons) do
					buttonTree:add(f["js_" .. js_i .. "_button_" .. i], v.buf, v.val)
				end
			end

			jstree:add(f.js_pov_count, struct.povCount)
			if (#struct.povs > 0) then
				local povTree = jstree:add("POVs:")
				for i, v in ipairs(struct.povs) do
					povTree:add(f["js_" .. js_i .. "_pov_" .. (i - 1)], v)
				end
			end

			js_i = js_i + 1
		elseif (struct.ID == 0x0f) then -- Date
			local dateTree = subtree:add(f.date, struct.buf)
			dateTree:set_text(struct.text)
			dateTree:add(f.date_usec, struct.usec)
			dateTree:add(f.date_sec, struct.sec)
			dateTree:add(f.date_min, struct.min)
			dateTree:add(f.date_hour, struct.hour)
			dateTree:add(f.date_day, struct.day)
			dateTree:add(f.date_month, struct.month)
			dateTree:add(f.date_year, struct.year)
		elseif (struct.ID == 0x10) then -- TZ
			local tzTree = subtree:add(f.date_tz, struct.buf)
			tzTree:set_text(struct.text)
			tzTree:add(f.date_tz_tz, struct.tz)
		end
		restData = struct.rest
	end

	local infoStr = ""
	local initial = true
	for i, v in pairs(info) do
		if (not initial) then
			infoStr = infoStr .. "  "
		end
		infoStr = infoStr .. i .. ": " .. v
		initial = false
	end

	pkt.cols.info:set("DS->RoboRIO   " .. infoStr)

end

udp_table = DissectorTable.get("udp.port")
udp_table:add(1110, FRC_DS)
udp_table:add(1115, FRC_DS)


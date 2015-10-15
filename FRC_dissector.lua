--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

local FRC_DS = Proto("FRC_DS", "FRC Driver Station")

local f = FRC_DS.fields
f.seqNum = ProtoField.uint16("FRC_DS.seq", "Sequence Number", base.DEC)
f.control = ProtoField.uint8("FRC_DS.control", "Control Byte")
f.mode = ProtoField.uint8("FRC_DS.mode", "Mode")
f.enabled = ProtoField.bool("FRC_DS.enabled", "Enabled")
f.estop = ProtoField.bool("FRC_DS.estop", "Emergency Stop")
f.unknown = ProtoField.uint8("FRC_DS.unknown", "Unknown field", base.HEX)
f.action = ProtoField.uint8("FRC_DS.action", "Action", base.HEX)
f.alliance_color = ProtoField.string("FRC_DS.alliance.color", "Alliance color")
f.alliance_pos = ProtoField.uint8("FRC_DS.alliance.pos", "Alliance position")
f.protocol = ProtoField.uint8("FRC_DS.protocol", "Protocol Version?")
f.date_nsec = ProtoField.uint32("FRC_DS.date.nanosecond", "Nanoseconds?")
f.date_sec = ProtoField.uint8("FRC_DS.date.second", "Second")
f.date_min = ProtoField.uint8("FRC_DS.date.minute", "Minute")
f.date_hour = ProtoField.uint8("FRC_DS.date.hour", "Hour")
f.date_day = ProtoField.uint8("FRC_DS.date.day", "Day")
f.date_month = ProtoField.uint8("FRC_DS.date.month", "Month")
f.date_year = ProtoField.uint8("FRC_DS.date.year", "Year")
f.date_tz_size = ProtoField.uint8("FRC_DS.date.tz_size", "Timezone Size")
f.date_tz_raw = ProtoField.bytes("FRC_DS.date.tz_raw", "Raw Timezone")
f.date_tz = ProtoField.string("FRC_DS.date.tz", "Timezone")
f.rest = ProtoField.bytes("FRC_DS.rest", "Rest")

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

function parseJoystick(buf)
	local out = {}
	out.size = buf(0, 1):uint() + 1 -- To include this byte
	out.header = buf(1, 1):uint()
	if (out.header ~= 0x0c) then
		return out
	end
	axisCount = buf(2, 1):uint()
	out.axes = {}
	local offset = 2
	for i = 1, axisCount do
		table.insert(out.axes, i, buf(offset + i, 1):int())
	end
	offset = offset + axisCount + 1
	local buttonCount = buf(offset, 1):uint()
	local buttonByteCount = math.floor(buttonCount / 8)
	if (buttonCount % 8 ~= 0) then
		buttonByteCount = buttonByteCount + 1
	end
	out.buttonByteCount = buttonByteCount
	out.buttons = {}
	out.buttonsRaw = {}
	for i = 1, buttonByteCount do
		table.insert(out.buttonsRaw, i, buf(offset + (buttonByteCount - i + 1), 1):uint())
		local mb = 8
		if (i == buttonByteCount) then
			mb = buttonCount % 8
		end
		for j = 1, mb do
			table.insert(out.buttons, (i - 1) * 8 + j, bit.band(bit.rshift(out.buttonsRaw[i], j - 1), 1))
		end
	end
	offset = offset + buttonByteCount + 1
	local povCount = buf(offset, 1):uint()
	out.povs = {}
	for i = 1, povCount do
		table.insert(out.povs, i, buf(offset + 1  + (i - 1) * 2, 2):int())
	end
	return out
end

function FRC_DS.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_DS")
	local subtree = tree:add(FRC_DS, buf())

	subtree:add(f.seqNum, buf(0,2))
	--subtree:add(f.unknown, buf(2,1)) -- Hide, it /always/ seems to be 0x01

	local modeBuf = buf(3, 1)
	local modeRaw = modeBuf:uint()

	local modeText = {"TeleOp", "Test", "Autonomous"}
	local enabledText = {"Disabled", "Enabled"}
	local estopText = {"Running", "Emergency Stopped"}

	local mode = {}
	mode.num = bit.band(modeRaw, 3)
	mode.text = modeText[mode.num + 1]
	local enabled = {}
	enabled.num = bit.band(bit.rshift(modeRaw, 2), 1)
	enabled.text = enabledText[enabled.num + 1]

	local estop = {}
	estop.num = bit.band(bit.rshift(modeRaw, 7), 1)
	estop.text = estopText[estop.num + 1]

	local modeExtraText = ""
	if (estop.num == 0) then
		modeExtraText = modeExtraText .. mode.text .. " " .. enabled.text
	else
		modeExtraText = modeExtraText .. estop.text
	end
	local modetree = subtree:add(f.control, modeBuf, modeRaw)
	modetree:append_text(" (" .. modeExtraText .. ")")

	modetree:add(f.mode, modeBuf, mode.num):set_text("......" .. bit.band(bit.rshift(mode.num, 1), 1) .. bit.band(mode.num, 1) .. " (" .. mode.text .. ")")
	modetree:add(f.enabled, modeBuf, enabled.num):set_text("....." .. enabled.num .. ".. (" .. enabled.text .. ")")
	modetree:add(f.estop, modeBuf, estop.num):set_text("" .. estop.num .. "....... (" .. estop.text .. ")")

	local actionText = {[0x00] = "Connect", [0x04] = "Unknown", [0x10] = "noop", [0x14] = "Restart Code", [0x18] = "Restart RoboRIO"}
	local action = buf(4,1):uint()
	subtree:add(f.action, buf(4, 1)):append_text(" (" .. actionText[action] .. ")")

	local allianceBuf = buf(5, 1)
	local allianceRaw = allianceBuf:uint()
	local alliance = {color = "Unknown", pos = allianceRaw}
	if allianceRaw < 3 then
		alliance.color = "Red"
		alliance.pos = allianceRaw + 1 -- 00, 01, 02
	elseif allianceRaw < 6 then
		alliance.color = "Blue"
		alliance.pos = allianceRaw - 2 -- 02, 03, 04
	end
	subtree:add(f.alliance_color, allianceBuf, alliance.color)
	subtree:add(f.alliance_pos, allianceBuf, alliance.pos)

	local rest = buf(6)
	local js_count = 0
	local joysticks = {}
	local restData = ""
	while rest:len() > 0 do
		local js = parseJoystick(rest)
		if (js.header == 0x0c) then
			js_count = js_count + 1
			table.insert(joysticks, js_count, js)
			if (js.size + 1 >= rest:len()) then
				break
			end
			rest = rest(js.size)
		else
			restData = rest
			break
		end
	end
	subtree:add(f.js_count, js_count)

	local offset = 6
	for i = 1, js_count do
		local js = joysticks[i]
		local jstree = subtree:add(nil, buf(offset, js.size), i):set_text("Joystick #" .. (i - 1) .. ":")

		offset = offset + 2

		jstree:add(f.js_axis_count, buf(offset, 1))
		if (#js.axes > 0) then
			local axistree = jstree:add("Axes:", buf(offset, 1 + #js.axes))
			for j, v in ipairs(js.axes) do
				axistree:add(f["js_" .. (i - 1) .. "_axis_" .. (j - 1)], buf(offset + j, 1), v)
			end
		end

		offset = offset + #js.axes + 1
		jstree:add(f.js_button_count, buf(offset, 1), #js.buttons)
		if (#js.buttons > 0) then
			local buttontree = jstree:add("Buttons:")
			for j, v in ipairs(js.buttons) do
				buttontree:add(f["js_" .. (i - 1) .. "_button_" .. j], buf(offset + 1 + math.floor((j - 1) / 8), 1), v)
			end
		end

		offset = offset + js.buttonByteCount + 1
		jstree:add(f.js_pov_count, buf(offset, 1), #js.povs)
		if (#js.povs > 0) then
			local povtree = jstree:add("POVs:")
			for j, v in ipairs(js.povs) do
				povtree:add_le(f["js_" .. (i - 1) .. "_pov_" .. (j - 1)], buf(offset + j, 2), v)
			end
		end

		offset = offset + 2 * #js.povs + 1

	end

	if (restData:len() > 0) then
		subtree:add(f.protocol, buf(offset + 1, 1))
		local datetree = subtree:add("Date Info:", buf(offset))
		datetree:add(f.date_nsec, buf(offset + 2, 4))
		offset = offset + 6
		datetree:add(f.date_sec, buf(offset, 1))
		datetree:add(f.date_min, buf(offset + 1, 1))
		datetree:add(f.date_hour, buf(offset + 2, 1))
		datetree:add(f.date_day, buf(offset + 3, 1))
		datetree:add(f.date_month, buf(offset + 4, 1))
		datetree:add(f.date_year, buf(offset + 5, 1))
		offset = offset + 6
		local tzSize = buf(offset, 1):uint()
		datetree:add(f.date_tz_size, buf(offset, 1))
		-- datetree:add(f.date_tz_raw, buf(offset + 1, tzSize))
		datetree:add(f.date_tz, buf(offset + 2, tzSize - 1)) -- Skip the first byte which always seems to be 0x10
	end

	pkt.cols.info:set("Mode: " .. modeExtraText .. "  Alliance: " .. alliance.color .. " " .. alliance.pos .. "  Action: " .. actionText[action])

end

udp_table = DissectorTable.get("udp.port")
udp_table:add(1110, FRC_DS)

------------------------------------------------------------------------------------------------

local FRC_RoboRIO = Proto("FRC_RoboRIO", "FRC RoboRIO")

local f = FRC_RoboRIO.fields
f.seqNum = ProtoField.uint16("FRC_RoboRIO.seq", "Sequence Number", base.DEC)
f.unknown1 = ProtoField.uint8("FRC_RoboRIO.unknown1", "Unknown field 1", base.HEX)
f.control = ProtoField.uint8("FRC_RoboRIO.control", "Control Byte")
f.mode = ProtoField.uint8("FRC_RoboRIO.mode", "Mode")
f.enabled = ProtoField.bool("FRC_RoboRIO.enabled", "Enabled")
f.connecting = ProtoField.bool("FRC_RoboRIO.connecting", "Connecting")
f.brownout = ProtoField.bool("FRC_RoboRIO.brownout", "Voltage Brownout")
f.estop = ProtoField.bool("FRC_RoboRIO.estop", "Emergency Stop")
f.codestatus = ProtoField.uint8("FRC_RoboRIO.codestatus", "Code Status", base.HEX)
f.battery = ProtoField.string("FRC_RoboRIO.battery", "Battery")
f.unknown2 = ProtoField.uint8("FRC_RoboRIO.unknown2", "Unknown field 2", base.HEX)

for js_i = 0, 5 do
	local fieldName = "FRC_RoboRIO.joystick." .. js_i
	f["js_" .. js_i] =  ProtoField.bytes(fieldName, "Joystick #" .. js_i)

	f["js_" .. js_i .. "_out"] =  ProtoField.bytes(fieldName , "Outputs")
	for i = 1, 32 do
		f["js_" .. js_i .. "_out_" .. i] =  ProtoField.bool(fieldName .. ".out." .. i, "Output #" .. i)
	end

	f["js_" .. js_i .. "_rumble_left"] =  ProtoField.uint16(fieldName .. ".rumble.left", "Rumble Left")
	f["js_" .. js_i .. "_rumble_right"] =  ProtoField.uint16(fieldName .. ".rumble.right", "Rumble Right")

end

f.pkt04 = ProtoField.bytes("FRC_RoboRIO.pkt04", "Unknown Pkt 1")
f.pkt04_unknown = ProtoField.bytes("FRC_RoboRIO.pkt04.unknown", "Unknown")

f.cpu = ProtoField.bytes("FRC_RoboRIO.CPU", "CPU Info")
f.cpu_count = ProtoField.uint8("FRC_RoboRIO.CPU.count", "Number of Cores")
f.cpu_0 = ProtoField.float("FRC_RoboRIO.CPU.0", "CPU0 %")
f.cpu_unknown1 = ProtoField.bytes("FRC_RoboRIO.cpu.unknown1", "Unknown 1")
f.cpu_unknown2 = ProtoField.float("FRC_RoboRIO.cpu.unknown2", "Unknown 2")
f.cpu_1 = ProtoField.float("FRC_RoboRIO.CPU.1", "CPU1 %")
f.cpu_unknown3 = ProtoField.bytes("FRC_RoboRIO.cpu.unknown3", "Unknown 3")
f.cpu_unknown4 = ProtoField.float("FRC_RoboRIO.cpu.unknown4", "Unknown 4")

f.pkt06 = ProtoField.bytes("FRC_RoboRIO.pkt06", "Unknown Pkt 2")
f.pkt06_unknown = ProtoField.bytes("FRC_RoboRIO.pkt06.unknown", "Unknown")

f.can_unknown = ProtoField.bytes("FRC_RoboRIO.CAN.unknown", "Unknown")
f.can_utilization = ProtoField.uint8("FRC_RoboRIO.CAN.utilization", "Utilization %")
f.can_busoff = ProtoField.uint8("FRC_RoboRIO.CAN.bus_off", "Bus Off")
f.can_txfull = ProtoField.uint8("FRC_RoboRIO.CAN.tx_full", "TX Full")
f.can_receive = ProtoField.uint8("FRC_RoboRIO.CAN.receive", "Receive")
f.can_transmit = ProtoField.uint8("FRC_RoboRIO.CAN.transmit", "Transmit")

f.rest = ProtoField.bytes("FRC_RoboRIO.rest", "Rest")

function FRC_RoboRIO.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_RoboRIO")
	local subtree = tree:add(FRC_RoboRIO, buf())

	subtree:add(f.seqNum, buf(0,2))
	-- subtree:add(f.unknown1, buf(2,1))

	local modeBuf = buf(3, 1)
	local modeRaw = buf(3, 1):uint()

	local modeText = {"TeleOp", "Test", "Autonomous"}
	local enabledText = {"Disabled", "Enabled"}
	local connectingText = {"Idle", "Connecting"}
	local brownoutText = {"Voltage: Normal", "Voltage: Brownout"}
	local estopText = {"Running", "Emergency Stopped"}

	local mode = {}
	mode.num = bit.band(modeRaw, 3)
	mode.text = modeText[mode.num + 1]

	local enabled = {}
	enabled.num = bit.band(bit.rshift(modeRaw, 2), 1)
	enabled.text = enabledText[enabled.num + 1]

	local connecting = {}
	connecting.num = bit.band(bit.rshift(modeRaw, 3), 1)
	connecting.text = connectingText[connecting.num + 1]

	local brownout = {}
	brownout.num = bit.band(bit.rshift(modeRaw, 4), 1)
	brownout.text = brownoutText[brownout.num + 1]

	local estop = {}
	estop.num = bit.band(bit.rshift(modeRaw, 7), 1)
	estop.text = estopText[estop.num + 1]

	local modeExtraText = ""
	if (estop.num == 0) then
		modeExtraText = modeExtraText .. mode.text .. " " .. enabled.text
	else
		modeExtraText = modeExtraText .. estop.text
	end
	local modetree = subtree:add(f.control, modeBuf, modeRaw)
	modetree:append_text(" (" .. modeExtraText .. ")")

	modetree:add(f.mode, modeBuf, mode.num):set_text("......" .. bit.band(bit.rshift(mode.num, 1), 1) .. bit.band(mode.num, 1) .. " (" .. mode.text .. ")")
	modetree:add(f.enabled, modeBuf, enabled.num):set_text("....." .. enabled.num .. ".. (" .. enabled.text .. ")")
	modetree:add(f.connecting, modeBuf, connecting.num):set_text("...." .. connecting.num .. "... (" .. connecting.text .. ")")
	modetree:add(f.brownout, modeBuf, brownout.num):set_text("..." .. brownout.num .. ".... (" .. brownout.text .. ")")
	modetree:add(f.estop, modeBuf, estop.num):set_text("" .. estop.num .. "....... (" .. estop.text .. ")")

	local codestatusBuf = buf(4, 1)
	local codeStatus = {buf = buf(4, 1)}
	codeStatus.raw = codeStatus.buf:uint()
	codeStatus.text = "Unknown"
	if (codeStatus.raw == 0x10) then
		codeStatus.text = "No Robot Code"
	elseif (codeStatus.raw == 0x30) then
		codeStatus.text = "Okay"
	end
	subtree:add(f.codestatus, buf(4,1), codeStatus.raw):append_text(" (" .. codeStatus.text .. ")")

	local batteryLevel = string.format("%.2d.%02d", buf(5,1):uint(), math.floor(99 * buf(6,1):uint() / 255)) -- Maybe
	subtree:add(f.battery, buf(5, 2), batteryLevel)

	-- subtree:add(f.unknown2, buf(7,1))

	local restData = buf(8)
	local js_i = 0
	while (restData:len() > 0) do
		local size = restData(0, 1):uint()
		local pktID = restData(1, 1):uint()
		if (pktID == 0x01) then -- Joystick Output
			local jstree = subtree:add(f["js_" .. js_i], restData(0, size + 1))
			if (size > 2) then
				local outtree = jstree:add(f["js_" .. js_i .. "_out"], restData(2, 4))
				for i = 0, 31 do
					local val = restData(5 - math.floor(i / 8), 1)
					outtree:add(f["js_" .. js_i .. "_out_" .. (i + 1)], val, bit.band(bit.rshift(val:uint(), i % 8), 1))
				end
				jstree:add(f["js_" .. js_i .. "_rumble_left"], restData(6, 2))
				jstree:add(f["js_" .. js_i .. "_rumble_right"], restData(8, 2))
			end
			js_i = js_i + 1
		elseif (pktID == 0x04) then -- Unknown 1
			local pkttree = subtree:add(f.pkt04, restData(0, size + 1))
			pkttree:add(f.pkt04_unknown, restData(2, size - 1))
		elseif (pktID == 0x05) then -- CPU Info
			local cputree = subtree:add(f.cpu, restData(0, size + 1))
			cputree:add(f.cpu_count, restData(2, 1))
			cputree:add(f.cpu_0, restData(3, 4))
			cputree:add(f.cpu_unknown1, restData(7, 8))
			cputree:add(f.cpu_unknown2, restData(15, 4))
			cputree:add(f.cpu_1, restData(19, 4))
			cputree:add(f.cpu_unknown3, restData(23, 8))
			cputree:add(f.cpu_unknown4, restData(31, 4))
		elseif (pktID == 0x06) then -- Unknown 2
			local pkttree = subtree:add(f.pkt06, restData(0, size + 1))
			pkttree:add(f.pkt06_unknown, restData(2, size - 1))
		elseif (pktID == 0x0e) then -- CAN Metrics
			local cantree = subtree:add("CAN Metrics", restData(0, size + 1))
			cantree:add(f.can_unknown, restData(2, 9))
			cantree:add(f.can_utilization, restData(11, 1))
			cantree:add(f.can_busoff, restData(12, 1))
			cantree:add(f.can_txfull, restData(13, 1))
			cantree:add(f.can_receive, restData(14, 1))
			cantree:add(f.can_transmit, restData(15, 1))
		else
			break
		end
		if (restData:len() > size + 1) then
			restData = restData(size + 1)
		else
			restData = ""
			break
		end
	end

	if (restData:len() > 0) then
		subtree:add(f.rest, restData)
	end


	pkt.cols.info:set("Mode: " .. modeExtraText .. "  Battery: " .. batteryLevel .. "  Status: " .. codeStatus.text)

end

udp_table = DissectorTable.get("udp.port")
udp_table:add(1150, FRC_RoboRIO)

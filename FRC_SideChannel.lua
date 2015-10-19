--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

local FRC_SideChannel = Proto("FRC_SideChannel", "FRC SideChannel")

local f = FRC_SideChannel.fields

f.size = ProtoField.uint8("FRC_SideChannel.size", "Size")
f.id = ProtoField.uint8("FRC_SideChannel.id", "ID", base.HEX)
f.msg = ProtoField.string("FRC_SideChannel.msg", "Message")
f.faults = ProtoField.bytes("FRC_SideChannel.faults", "Faults")
f.faults_comms = ProtoField.uint16("FRC_SideChannel.faults.comms", "Comms")
f.faults_12V = ProtoField.uint16("FRC_SideChannel.faults.12v", "12V")
f.faults_6V = ProtoField.uint16("FRC_SideChannel.faults.6v", "6V")
f.faults_5V = ProtoField.uint16("FRC_SideChannel.faults.5v", "5V")
f.faults_3_3V = ProtoField.uint16("FRC_SideChannel.faults.3v3", "3.3V")
f.js = ProtoField.bytes("FRC_SideChannel.js", "Joystick")
f.js_idx = ProtoField.uint8("FRC_SideChannel.js.index", "Index")
f.js_connected = ProtoField.bool("FRC_SideChannel.js.connect", "Connected")
f.js_len = ProtoField.uint8("FRC_SideChannel.js.len", "Length")
f.js_name = ProtoField.string("FRC_SideChannel.js.name", "Name")
f.js_axis_count = ProtoField.uint8("FRC_SideChannel.js.axisCount", "Axis Count")
f.js_axis_enum = ProtoField.bytes("FRC_SideChannel.js.axisEnum", "Axis Enum?")
f.js_button_count = ProtoField.uint8("FRC_SideChannel.js.buttonCount", "Button Count")
f.js_pov_count = ProtoField.uint8("FRC_SideChannel.js.povCount", "POV Count")


local function validID(id)
	return (id == 0x00 or id == 0x04 or id ~= 0x05)
end

local max_msg_len = 4096 -- Need to test the actual max length (if there is one)

local function dissect(buf, pkt, tree)

	if (buf:len() < 2) then
		return 0, -1
	end

	local pktBuf = buf(bytes_consumed)
	local sizeBuf = pktBuf(0, 2)
	local size = sizeBuf:uint()

	if (size > max_msg_len) then
		return 1, 0 -- Nom a byte
	end

	if (buf:len() < (size + 2)) then
		return 0, (size + 2) - buf:len() -- Need more bytes
	end

	local subtree = tree:add(FRC_SideChannel, buf(0, size + 2))

	subtree:add(f.size, sizeBuf)

	if (size == 0) then
		return 2, 0 -- Empty
	end

	local idBuf = buf(2, 1)
	local id = idBuf:uint()
	local data = buf(3, size - 1)

	subtree:add(f.id, idBuf)

	if (id == 0x00) then -- Message
		subtree:add(f.msg, data)
	elseif (id == 0x02) then -- Joystick
		local jsTree = subtree:add(f.js, data)
		local idx = data(0, 1)
		local connected = data(1, 1)
		-- data(2, 1) -- 0x01 connected, 0xff disconnected
		local length = data(3, 1)
		local name = data(4, length:uint())
		-- data(4 + length, 3) -- unknow
		jsTree:add(f.js_idx, idx)
		jsTree:add(f.js_connected, connected)
		jsTree:add(f.js_len, length)
		jsTree:add(f.js_name, name)
		local offset = 4 + length:uint()
		local axisCount = data(offset, 1)
		jsTree:add(f.js_axis_count, axisCount)
		if (axisCount:uint() > 0) then
			jsTree:add(f.js_axis_enum, data(offset + 1, axisCount:uint()))
		end
		offset = offset + 1 + axisCount:uint()
		jsTree:add(f.js_button_count, data(offset, 1))
		jsTree:add(f.js_pov_count, data(offset + 1, 1))
		local text = "Joystick #" .. idx:uint() .. ":"
		if (length:uint() > 0) then
			local connectedText = {"Disconnected", "Connected"}
			text = text .. " \"" .. name:string() .. "\" (" .. connectedText[connected:uint() + 1] .. ")"
		end
		jsTree:set_text(text)
	elseif (id == 0x04) then --  Comms, 12V
		if (data:len() ~= 4) then
			return 1, 0 -- Invalid 0x04 packet
		end
		local faultsTree = subtree:add(f.faults, data)
		count_comms = data(0, 2)
		count_12v = data(2, 2)
		faultsTree:add(f.faults_comms, count_comms)
		faultsTree:add(f.faults_12V, count_12v)
		faultsTree:set_text("Faults:  Comms: " .. count_comms:uint() .. "  12V: " .. count_12v:uint())
	elseif (id == 0x05) then --  6V, 5V, 3.3V
		if (data:len() ~= 6) then
			return 1, 0 -- Invalid 0x06 packet
		end
		local faultsTree = subtree:add(f.faults, data) -- :set_text("Faults:")
		count_6v = data(0, 2)
		count_5v = data(2, 2)
		count_3v3 = data(4, 2)
		faultsTree:add(f.faults_6V, count_6v)
		faultsTree:add(f.faults_5V, count_5v)
		faultsTree:add(f.faults_3_3V, count_3v3)
		faultsTree:set_text("Faults:  6V: " .. count_6v:uint() .. "  5V: " .. count_5v:uint() .. "  3.3V: " .. count_3v3:uint())
	else
	end
	return 2 + size, 0
end

function FRC_SideChannel.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_SideChannel")

	if (buf:len() < 2) then
		pkt.desegment_len = DESEGMENT_ONE_MORE_SEGMENT
		return
	end

	local offset, needed, consumed = 0, 0, 1

	while (buf:len() - offset > 0) and consumed > 0 do
		consumed, needed = dissect(buf(offset), pkt, tree)
		offset = offset + consumed
	end

	if (consumed == 0) then -- Partial
		pkt.desegment_offset = offset
		if (needed < 0) then
			pkt.desegment_len = DESEGMENT_ONE_MORE_SEGMENT
		else
			pkt.desegment_len = needed
		end
	end
	return
end

tcp_table = DissectorTable.get("tcp.port")
tcp_table:add(1740, FRC_SideChannel)

--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

local FRC_NetworkTable = Proto("FRC_NetworkTable", "FRC NetworkTable")

local f = FRC_NetworkTable.fields

f.msgType = ProtoField.uint8("FRC_NetworkTable.msgType", "Message Type", base.HEX)
f.revision = ProtoField.uint16("FRC_NetworkTable.revision", "Protocol Revision", base.HEX)

f.entry_name = ProtoField.string("FRC_NetworkTable.entry.name", "Name")
f.entry_ID = ProtoField.uint16("FRC_NetworkTable.entry.ID", "ID")
f.entry_seqnum = ProtoField.uint16("FRC_NetworkTable.entry.seqnum", "Sequence Number")
f.entry_type = ProtoField.uint8("FRC_NetworkTable.entry.type", "Type")
f.entry_value = ProtoField.bytes("FRC_NetworkTable.entry.value", "Value")

local entryTypes = {}
entryTypes[0x00] = "Boolean"
entryTypes["Boolean"] = 0x00
entryTypes[0x01] = "Double"
entryTypes["Double"] = 0x01
entryTypes[0x02] = "String"
entryTypes["String"] = 0x02
entryTypes[0x10] = "Boolean Array"
entryTypes["Boolean Array"] = 0x10
entryTypes[0x11] = "Double Array"
entryTypes["Double Array"] = 0x11
entryTypes[0x12] = "String Array"
entryTypes["String Array"] = 0x12

function getEntryType(eType)
	local out = entryTypes[eType]
	if (out == nil) then
		if (type(eType) == "string") then
			out = 0xff
		else
			out = "Unknown"
		end
	end
	return out
end

function getValueBuf(buf, entryType)
	if (type(entryType) ~= "number") then
		entryType = getEntryType(entryType)
	end
	if (entryType == 0x00) then -- Boolean
		length = 1
	elseif (entryType == 0x01) then -- Double
		length = 8
	elseif (entryType == 0x02) then -- String
		if (buf:len() < 2) then
			return nil, -1
		end
		length = 2 + buf(0, 2):uint()
	elseif (bit.band(entryType, 0xf0) > 0) then -- Array
		if (buf:len() < 2) then
			return nil, -1
		end
		local arrayLength = buf(0, 2):uint()
		local total = 2
		local elmType = bit.band(entryType, 0x0f)
		if (elmType == getEntryType("String")) then
			for i = 1, arrayLength do
				if (buf:len() < total) then
					return nil, -1
				end
				local _, elmLen = getValueBuf(buf(total), elmType)
				if (elmLen < 0) then
					return nil, -1
				end
				total = total + elmLen
			end
		else
			local _, elmLen = getValueBuf(buf, elmType)
			total = total + arrayLength * elmLen
		end
		length = total
	end
	if (buf:len() < length) then
		return nil, length
	end
	return buf(0, length), length
end

local entries = {}

function dissect(buf, pkt, tree)

	local msgTypeBuf = buf(0, 1)
	msgType = msgTypeBuf:uint()

	local subtree = tree:add(FRC_NetworkTable, buf)
	local msgTypeField = subtree:add(f.msgType, msgTypeBuf)

	local consumed, needed = 0, 0

	local msgTypeText = {}
	msgTypeText[0x00] = "Keep Alive"
	msgTypeText[0x01] = "Client Hello"
	msgTypeText[0x02] = "Protocol Revision Unsupported"
	msgTypeText[0x03] = "Server Hello Complete"
	msgTypeText[0x10] = "Entry Assignment"
	msgTypeText[0x11] = "Entry Update"

	local typeText = msgTypeText[msgType]
	if (typeText == nil) then
		typeText = "~Unknown Message Type~"
	end

	subtree:set_text("FRC NetworkTable: " .. typeText)

	if (msgType == 0x00) then -- Keep Alive
		consumed = 1
	elseif (msgType == 0x01) then -- Client Hello
		if (buf:len() < 3) then
			needed = 3 - buf:len()
		else
			local revision = buf(1, 2)
			subtree:add_le(f.revision, revision)
			subtree:append_text(string.format(" (Revision: 0x%04x)", revision:le_uint()))
			consumed = 3
		end
	elseif (msgType == 0x02) then -- Protocol Revision Unsupported
		if (buf:len() < 3) then
			needed = 3 - buf:len()
		else
			local revision = buf(1, 2)
			subtree:add_le(f.revision, revision)
			subtree:append_text(string.format(" (Requires Revision: 0x%04x)", revision:uint()))
			consumed = 3
		end
	elseif (msgType == 0x03) then -- Server Hello Complete
		consumed = 1
	elseif (msgType == 0x10) then -- Entry Assignment
		if (buf:len() == 1) then
			return 0, -1
		end

		local nameBuf, nameLen = getValueBuf(buf(1), "String")
		if (nameBuf == nil) then
			return 0, -1
		end

		local name = nameBuf(2):string()
		consumed = consumed + 1 + nameBuf:len()

		if (buf:len() < (consumed + 5)) then
			return 0, -1
		end

		local eType = buf(consumed, 1)
		local id = buf(consumed + 1, 2)
		local seqNum = buf(consumed + 3, 2)
		consumed = consumed + 5

		subtree:append_text(string.format(": ID: %04x", id:uint()))
		subtree:append_text(string.format("  Name: \"%s\"", name))
		subtree:append_text(string.format("  Type: %02x (%s)", eType:uint(), getEntryType(eType:uint())))
		subtree:append_text(string.format("  SeqNum: %d", seqNum:uint()))

		if (buf:len() < (consumed + 1)) then
			return 0, -1
		end

		local value, len = getValueBuf(buf(consumed), eType:uint())

		if (value == nil) then
			return 0, -1
		end

		consumed = consumed + len

		subtree:add(f.entry_name, nameBuf(2))
		subtree:add(f.entry_type, eType):append_text(" (" .. getEntryType(eType) .. ")")
		subtree:add(f.entry_ID, id)
		subtree:add(f.entry_seqnum, seqNum)
		subtree:add(f.entry_value, value)

		if (not pkt.visited) then
			if (id:uint() ~= 0xffff) then
				entries[id:uint()] = {name=nameBuf(2):string(), eType=eType:uint()}
			end
		end

	elseif (msgType == 0x11) then -- Entry Update
		if (buf:len() < 2) then
			return 0, -1
		end
		local id = buf(1, 2)
		consumed = consumed + 3

		local entry = entries[id:uint()]
		local eType = entry.eType
		local name = entry.name

		if (buf:len() < (consumed + 2)) then
			return 0, -1
		end
		local seqNum = buf(consumed, 2)

		subtree:append_text(string.format(": ID: %04x", id:uint()))
		subtree:append_text(string.format("  Name: \"%s\"", name))
		subtree:append_text(string.format("  Type: %02x (%s)", eType, getEntryType(eType)))
		subtree:append_text(string.format("  SeqNum: %d", seqNum:uint()))

		local value, len = getValueBuf(buf(consumed + 2), eType)
		consumed = consumed + 2 + len

		if (value == nil) then
			return 0, -1
		end

		subtree:add(f.entry_name, name)
		subtree:add(f.entry_type, eType):append_text(" (" .. getEntryType(eType) .. ")")
		subtree:add(f.entry_ID, id)
		subtree:add(f.entry_seqnum, seqNum)
		subtree:add(f.entry_value, value)

		subtree:set_len(consumed)
	end

	return consumed, needed
end



function FRC_NetworkTable.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_NetworkTable")

	if (buf:len() < 1) then
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
tcp_table:add(1735, FRC_NetworkTable)


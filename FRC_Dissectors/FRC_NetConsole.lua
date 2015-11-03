--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

local FRC_NetConsole = Proto("FRC_NetConsole", "FRC NetConsole")

local f = FRC_NetConsole.fields

f.msg = ProtoField.string("FRC_NetConsole.msg", "Message")

function FRC_NetConsole.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_NetConsole")
	local subtree = tree:add(FRC_NetConsole, buf())
	subtree:add(f.msg, buf())

	local text = buf():string()
	if (text:len() > 100) then
		text = text:sub(0, 100) .. " ..."
	end
	pkt.cols.info:set("NetConsole: " .. text)
end

udp_table = DissectorTable.get("udp.port")
udp_table:add(6666, FRC_NetConsole)

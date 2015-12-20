--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

parser = require("FRC_Data_Structures")

local FRC_RoboRIO = Proto("FRC_RoboRIO", "FRC RoboRIO")

local f = FRC_RoboRIO.fields
f.seqNum = ProtoField.uint16("FRC_RoboRIO.seq", "Sequence Number", base.DEC)
f.unknown1 = ProtoField.uint8("FRC_RoboRIO.unknown1", "Unknown field 1", base.HEX)

f.control = ProtoField.uint16("FRC_RoboRIO.control", "Control Byte", base.HEX)
f.mode = ProtoField.uint8("FRC_RoboRIO.control.mode", "Mode")
f.enabled = ProtoField.bool("FRC_RoboRIO.control.enabled", "Enabled")
f.codeState = ProtoField.bool("FRC_RoboRIO.control.codeState", "Code State")
f.brownout = ProtoField.bool("FRC_RoboRIO.control.brownout", "Voltage Brownout")
f.estop = ProtoField.bool("FRC_RoboRIO.control.estop", "Emergency Stop")
f.DSConnected = ProtoField.bool("FRC_RoboRIO.control.DSConnected", "Driver Station Connected")
f.robotCode = ProtoField.bool("FRC_RoboRIO.control.robotCode", "Robot Code")
f.ctrlUnknown = ProtoField.uint16("FRC_RoboRIO.control.unknown", "Unknown")

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

f.cpu = ProtoField.bytes("FRC_RoboRIO.CPU", "CPU Info")
f.cpu_count = ProtoField.uint8("FRC_RoboRIO.CPU.count", "Number of CPUs")
for i = 0, 1 do
	f["cpu_" .. i ] = ProtoField.bytes("FRC_RoboRIO.CPU." .. i , "CPU" .. i .. "")
	f["cpu_" .. i .. "_percent"] = ProtoField.float("FRC_RoboRIO.CPU." .. i .. ".percent" , "CPU" .. i .. " %")
	f["cpu_" .. i .. "_unknown1"] = ProtoField.bytes("FRC_RoboRIO.CPU." .. i .. ".unknown1", "Unknown 1")
	f["cpu_" .. i .. "_unknown2"] = ProtoField.float("FRC_RoboRIO.CPU." .. i .. ".unknown2", "Unknown 2")
end

local unknownPkts = {0x04, 0x06}
for i, v in ipairs(unknownPkts) do
	local tmp = string.format("%02x", v)
	f["pkt_" .. tmp ] = ProtoField.bytes("FRC_RoboRIO.pkt_" .. tmp, "Unknown Pkt 0x" .. tmp)
	f["pkt_" .. tmp .. "_unknown"] = ProtoField.bytes("FRC_RoboRIO.pkt_" .. tmp .. ".unknown", "Unknown")
end

f.can = ProtoField.bytes("FRC_RoboRIO.CAN", "CAN Metrics")
f.can_unknown = ProtoField.bytes("FRC_RoboRIO.CAN.unknown", "Unknown")
f.can_utilization = ProtoField.uint8("FRC_RoboRIO.CAN.utilization", "Utilization %")
f.can_busoff = ProtoField.uint8("FRC_RoboRIO.CAN.bus_off", "Bus Off")
f.can_txfull = ProtoField.uint8("FRC_RoboRIO.CAN.tx_full", "TX Full")
f.can_receive = ProtoField.uint8("FRC_RoboRIO.CAN.receive", "Receive")
f.can_transmit = ProtoField.uint8("FRC_RoboRIO.CAN.transmit", "Transmit")

f.disk = ProtoField.bytes("FRC_RoboRIO.Disk", "Disk Info")
f.disk_free = ProtoField.uint32("FRC_RoboRIO.Disk.space", "Space Available (B)")

f.ram = ProtoField.bytes("FRC_RoboRIO.RAM", "RAM Info")
f.ram_free = ProtoField.uint32("FRC_RoboRIO.RAM.space", "Available (B)")

f.rest = ProtoField.bytes("FRC_RoboRIO.rest", "Rest")

function FRC_RoboRIO.dissector(buf, pkt, tree)
	pkt.cols.protocol:set("FRC_RoboRIO")
	local subtree = tree:add(FRC_RoboRIO, buf())
	local info = {}

	subtree:add(f.seqNum, buf(0,2))
	-- subtree:add(f.unknown1, buf(2,1))

	local control = parser.parseControlBytes(buf(3, 2))

	local controlText = ""
	if (control.estop.val == 0) then
		controlText = controlText .. control.mode.text .. " " .. control.enabled.text
	else
		controlText = "Emergency Stopped"
	end

	info["Mode"] = controlText

	local controlTree = subtree:add(f.control, control.buf )
	controlTree:append_text(" (" .. controlText .. ")")

	local items = {"raw", "mode", "enabled", "codeState", "brownout", "estop", "robotCode", "unknown"}
	for i, v in ipairs(items) do
		controlTree:add(f[v], control[v].buf, control[v].val):set_text(control[v].bitstr .. " (" .. control[v].name .. ": " .. control[v].text .. ")")
	end

	local batteryLevel = parser.parseBattery(buf(5, 2))
	subtree:add(f.battery, batteryLevel.buf, batteryLevel.val)
	info["Battery"] = batteryLevel.val

	-- subtree:add(f.unknown2, buf(7,1))

	local restData = buf(8)

	local js_i = 0
	while (restData:len() > 0) do
		local struct = parser.parseStructure(restData)
		if (struct.ID == 0x01) then -- Joystick Output
			local jstree = subtree:add(f["js_" .. js_i], struct.buf)
			if (struct.outputs ~= nil) then
				local outputsTree = jstree:add(f["js_" .. js_i .. "_out"], struct.outputsBuf)
				for i = 1, 32 do
					outputsTree:add(f["js_" .. js_i .. "_out_" .. i], struct.outputs[i].buf, struct.outputs[i].val)
				end
				jstree:add(f["js_" .. js_i .. "_rumble_left"], struct.rumble_left) -- .buf, struct.rumble_left.val)
				jstree:add(f["js_" .. js_i .. "_rumble_right"], struct.rumble_right) -- .buf, struct.rumble_right.val)
			end
			js_i = js_i + 1
		elseif (struct.ID == 0x04) then -- RAM Info
			local diskInfoTree = subtree:add(f.disk, struct.buf)
			diskInfoTree:set_text("Disk Info (" .. struct.disk.val .. " B)")
			diskInfoTree:add(f.disk_free, struct.disk.buf, struct.disk.val)
		elseif (struct.ID == 0x05) then -- CPU Info
			local cpuInfoTree = subtree:add(f.cpu, struct.buf)
			cpuInfoTree:add(f.cpu_count, struct.count)
			cpuInfoTree:set_text("CPU Info")
			for i = 1, struct.count:uint() do
				local cpu = struct.cpus[i]
				cpuTree = cpuInfoTree:add(f["cpu_" .. (i - 1)], cpu.buf)
				cpuTree:set_text("CPU #" .. (i - 1))
				cpuTree:add(f["cpu_" .. (i - 1) .. "_percent"], cpu.percent)
				cpuTree:add(f["cpu_" .. (i - 1) .. "_unknown1"], cpu.unknown1)
				cpuTree:add(f["cpu_" .. (i - 1) .. "_unknown2"], cpu.unknown2)
			end
		elseif (struct.ID == 0x06) then -- Disk Info
			local ramInfoTree = subtree:add(f.ram, struct.buf)
			ramInfoTree:set_text("RAM Info (" .. struct.ram.val .. " B)")
			ramInfoTree:add(f.ram_free, struct.ram.buf, struct.ram.val)
		elseif (struct.ID == 0x0e) then -- CAN Metrics
			local canTree = subtree:add(f.can, struct.buf)
			canTree:set_text("CAN Metrics")
			canTree:add(f.can_unknown, struct.unknown)
			canTree:add(f.can_utilization, struct.utilization)
			canTree:add(f.can_busoff, struct.busoff)
			canTree:add(f.can_txfull, struct.txfull)
			canTree:add(f.can_receive, struct.receive)
			canTree:add(f.can_transmit, struct.transmit)
		elseif (struct.ID == 0x04 or struct.ID == 0x06) then -- Unknown packets
			local tmp = string.format("%02x", struct.ID)
			local unknownTree = subtree:add(f["pkt_" .. tmp], struct.buf)
			unknownTree:set_text("Unknown Pkt 0x" .. tmp)
			unknownTree:add(f["pkt_" .. tmp .. "_unknown"], struct.val)
		else
			subtree:add(struct.text, struct.buf)
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

	pkt.cols.info:set("RoboRIO->DS   " ..  infoStr)

end

udp_table = DissectorTable.get("udp.port")
udp_table:add(1150, FRC_RoboRIO)

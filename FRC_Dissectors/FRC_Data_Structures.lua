--------------------------------------------------------------------------------
-- Copyright (c) Creighton 2015. All Rights Reserved.                         --
-- Open Source Software - May be modified and shared but must                 --
-- be accompanied by the license file in the root source directory            --
--------------------------------------------------------------------------------

local parser = {}

function parser.parseAllianceByte(buf)
	local out = {}
	out.buf = buf
	out.color = "Unknown"
	out.pos = buf:uint()

	if out.pos < 3 then
		out.color = "Red"
		out.pos = out.pos + 1 -- 00, 01, 02
	elseif out.pos < 6 then
		out.color = "Blue"
		out.pos = out.pos - 2 -- 03, 04, 05
	end

	out.text = out.color .. " " .. out.pos

	return out
end

function parser.parseBattery(buf)
	local out = {}
	out.buf = buf
	out.val = string.format("%.2d.%02d", buf(0,1):uint(), math.floor(99 * buf(1, 1):uint() / 255)) -- Maybe
	return out
end

function parser.parseControlBytes(buf)
	local out = {}
	out.buf = buf

	local bitLimit = 15
	if (buf:len() == 1) then
		bitLimit = bitLimit - 8
	end

	local fields = {}
	fields.estop = {bitnum=0, name="E-Stopped", text={"No", "Yes"}}                  -- 1000 0000  0000 0000
	fields.DSConnected = {bitnum=2, name="DS", text={"Disconnected", "Connected"}}   -- 0010 0000  0000 0000
	fields.brownout = {bitnum=3, name="Voltage", text={"Normal", "Brownout"}}        -- 0001 0000  0000 0000
	fields.codeState = {bitnum=4, name="Code", text={"Idle", "Initializing"}}        -- 0000 1000  0000 0000
	fields.FMSConnected = {bitnum=4, name="FMS", text={"Disconnected", "Connected"}} -- 0000 1000  0000 0000
	fields.enabled = {bitnum=5, name="State", text={"Disabled", "Enabled"}}          -- 0000 0100  0000 0000
	fields.mode = {bitnum=6, size=2, name="Mode"}                                    -- 0000 0011  0000 0000
	fields.mode.text = {"TeleOp", "Test", "Autonomous", "Unknown"}
	fields.robotCode = {bitnum=10, name="Robot Code", text={"No", "Yes"}}            -- 0000 0000  0010 0000
	fields.rebootRoboRIO = {bitnum=12, name="Reboot", text={"No", "Yes"}}            -- 0000 0000  0000 1000
	fields.restartRobotCode = {bitnum=13, name="Restart Code", text={"No", "Yes"}}   -- 0000 0000  0000 0100
	fields.unknown = {mask=0x40c3, name="Unknown"}                                   -- 0110 0000  1100 0011

	out.raw = {buf=out.buf, name="Raw", val=buf:uint(), bitstr=""}
	out.raw.text = string.format("0x%02x", out.raw.val)
	for i = 0, bitLimit do
		if (i > 0 and i % 8 == 0) then
			out.raw.bitstr = out.raw.bitstr .. " "
		end
		out.raw.bitstr = out.raw.bitstr .. out.buf:bitfield(i, 1)
	end

	for k, v in pairs(fields) do
		local size = 1
		if (v.size ~= nil) then
			size = v.size
		end
		local tmp = {name=v.name}

		if (k ~= "unknown") then
			if (v.bitnum <= bitLimit) then
				tmp.val = out.buf:bitfield(v.bitnum, size)
				tmp.buf = buf(math.floor(v.bitnum / 8), 1)

				local str = string.rep(".", v.bitnum)
				for i = 1, size do
					str = str .. bit.band(bit.rshift(tmp.val, size - i), 1)
				end
				str = str .. string.rep(".", (bitLimit + 1) - (v.bitnum + size))
				if (bitLimit > 7) then
					str = str:sub(1, 8) .. " " .. str:sub(9, 16)
				end
				tmp.text = v.text[tmp.val + 1]
				tmp.bitstr = str
				out[k] = tmp
			end
		else
			local mask = bit.rshift(v.mask, 15 - bitLimit)
			tmp.val = bit.band(out.buf:uint(), mask)
			local str = ""
			for i = 0, bitLimit do
				local bitval = bit.band(bit.rshift(tmp.val, bitLimit - i), 1)
				local maskbit = bit.band(bit.rshift(mask, bitLimit - i), 1)
				if (bitval == 1) then
					str = str .. "1"
				elseif (maskbit == 1) then
					str = str .. "0"
				else
					str = str .. "."
				end
			end
			if (bitLimit > 7) then
				str = str:sub(1, 8) .. " " .. str:sub(9, 16)
			end
			tmp.bitstr = str
			tmp.text = string.format("0x%x", tmp.val)
			out[k] = tmp
		end
	end

	return out
end

function parser.parseStructure(buf)
	local out = {}
	local size = buf(0, 1):uint()
	local ID = buf(1, 1):uint()

	out.size = size
	out.ID = ID
	out.buf = buf(0, size + 1)

	if (ID == 0x01) then -- Joystick Output
		out.text = "Joystick Output"
		if (size > 2) then
			out.outputs = {}
			out.outputsBuf = buf(2, 4)
			for i = 0, 31 do
				local tmp = {}
				tmp.buf = buf(5 - math.floor(i / 8), 1)
				tmp.val = bit.band(bit.rshift(tmp.buf:uint(), i % 8), 1)
				table.insert(out.outputs, tmp)
			end
			out.rumble_left = buf(6, 2) -- {buf=buf(6, 2), val=buf(6, 2)}
			out.rumble_right = buf(8, 2) -- {buf=buf(8, 2), val=buf(8, 2)}
		end
	elseif (ID == 0x04) then -- Disk Info
		out.text = "Disk Info"
		local tmp = {}
		tmp.buf = buf(6, 4)
		tmp.val = tmp.buf:uint();
		out.disk = tmp;
	elseif (ID == 0x05) then -- CPU Info
		out.text = "CPU Info"
		out.count = buf(2, 1)
		out.cpus = {}
		for i = 0, out.count:uint() - 1 do
			local tmp = {}
			tmp.buf = buf(3 + i * 16, 12)
			tmp.percent = buf(3 + i * 16, 4)
			tmp.unknown1 = buf(7 + i * 16, 8)
			tmp.unknown2 = buf(15 + i * 16, 4)
			table.insert(out.cpus, tmp)
		end
	elseif (ID == 0x06) then -- RAM Info
		out.text = "RAM Info"
		local tmp = {}
		tmp.buf = buf(6, 4)
		tmp.val = tmp.buf:uint();
		out.ram = tmp;
	elseif (ID == 0x0c) then -- Joystick
		out.text = "Joystick"
		local offset = 2
		out.axisCount = buf(offset, 1)
		out.axisBuf = buf(offset, 1 + out.axisCount:uint())
		out.axes = {}
		if (out.axisCount:uint() > 0) then
			for i = 1, out.axisCount:uint() do
				table.insert(out.axes, buf(offset + i, 1))
			end
		end
		offset = offset + out.axisCount:uint() + 1
		out.buttonCount = buf(offset, 1)
		local buttonByteCount = math.floor(out.buttonCount:uint() / 8)
		if (out.buttonCount:uint() % 8 ~= 0) then
			buttonByteCount = buttonByteCount + 1
		end
		out.buttonByteCount = buttonByteCount
		out.buttonBuf = buf(offset, 1 + out.buttonByteCount)
		out.buttons = {}
		if (out.buttonByteCount > 0) then
			for i = 1, buttonByteCount do
				local mb = 8
				if (i == buttonByteCount) then
					mb = out.buttonCount:uint() % 8
				end
				for j = 1, mb do
					local tmp = {}
					tmp.buf = buf(offset + 1 + (buttonByteCount - i), 1)
					tmp.val = bit.band(bit.rshift(tmp.buf:uint(), j - 1), 1)
					table.insert(out.buttons, tmp)
				end
			end
			offset = offset + buttonByteCount
		end
		offset = offset + 1
		out.povCount = buf(offset, 1)
		out.povBuf = buf(offset, 1 + out.povCount:uint())
		out.povs = {}
		if (out.povCount:uint() > 0) then
			for i = 1, out.povCount:uint() do
				table.insert(out.povs, i, buf(offset + 1 + (i - 1) * 2, 2))
			end
		end
	elseif (ID == 0x0e) then -- CAN Metrics
		out.text = "CAN Metrics"
		out.unknown = buf(2, 9)
		out.utilization = buf(11, 1)
		out.busoff = buf(12, 1)
		out.txfull = buf(13, 1)
		out.receive = buf(14, 1)
		out.transmit = buf(15, 1)
	elseif (ID == 0x0f) then -- Date
		local date = parser.parseDate(buf(2))
		for k, v in pairs(date) do
			out[k] = v
		end
	elseif (ID == 0x10) then -- Timezone
		out.text = "Timezone"
		out.tz = buf(2, size - 1)
	else
		out.text = "New Unknown Structure"
	end

	if (buf:len() > size + 1) then
		out.rest = buf(size + 1)
	else
		out.rest = ""
	end

	return out
end

function parser.parseDate(buf)
	local out = {}
	out.text = "Date"
	out.usec = buf(0, 4)
	out.sec = buf(4, 1)
	out.min =  buf(5, 1)
	out.hour = buf(6, 1)
	out.day = buf(7, 1)
	out.month = buf(8, 1)
	out.year = buf(9, 1)
	out.dateText = string.format("%04d-%02d-%02d", out.year:uint() + 1900, out.month:uint() + 1, out.day:uint())
	out.dateText = out.dateText .. " " .. string.format("%02d:%02d:%02d.%03d", out.hour:uint(), out.min:uint(), out.sec:uint(), out.usec:uint() / 1000 )
	return out
end

return parser

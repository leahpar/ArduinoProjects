-- must switch to 9600 baud to control relay
uart.setup(0,9600,8,0,1)

-- join wifi
wifi.setmode(wifi.STATION)

wifi.sta.config(ssid="TATOOINE", pwd="yapademotdepasse")

-- print(wifi.sta.getip())
print("hostname: "..wifi.sta.gethostname())

-- Lua web server
srv=net.createServer(net.TCP)
srv:listen(80,function(conn)
    conn:on("receive", function(client,request)

	-- print(request);


	local _, _, method, path, vars = string.find(request, "([A-Z]+) (.+)?(.+) HTTP");



        if (method == nil) then
            _, _, method, path = string.find(request, "([A-Z]+) (.+) HTTP");
        end


        local _GET = {}
        if (vars ~= nil) then
            for k, v in string.gmatch(vars, "(%w+)=(%w+)&*") do
                _GET[k] = v
            end
	end

        if (_GET.relay == "ON") then
              uart.write( 0, 0xA0,0x01,0x01,0xA2);
        end
        if (_GET.relay == "OFF") then
              uart.write( 0, 0xA0,0x01,0x00,0xA1);
        end

        local buf = "HTTP/1.1 200 OK\r\n\r\n";
        buf = buf.."<html><body><h1>Relay control</h1>";
        buf = buf.."Switch relay <a href=\"?relay=ON\"><button>ON</button></a><a href=\"?relay=OFF\"><button>OFF</button></a></body></html>";
	client:send(buf);

    end)
    conn:on("sent",function(conn)
	conn:close();
	collectgarbage();
    end)
end)

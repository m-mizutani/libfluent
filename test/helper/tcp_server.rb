#!/usr/bin/env ruby

require "socket"
require "msgpack"
require "pp"

gs = TCPServer.open(24224)
socks = [gs]

sock = gs.accept
unpkr = MessagePack::Unpacker.new(sock)

begin
  unpkr.each do |msg|
    print(msg[0], " ", msg[1], " ", msg[2].to_s, "\n")
    # PP.pp(msg[2], STDOUT)
    STDOUT.flush
  end
rescue Interrupt
  # ignore
end

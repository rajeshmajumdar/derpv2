package handlers

import (
	"log"
	"net"
)

func StartDiscoveryListener(serverName string) {
	addr, err := net.ResolveUDPAddr("udp", ":5555")
	if err != nil {
		log.Println("[discovery] Failed to resolve UDP address:", err)
		return
	}

	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		log.Println("[discovery] Failed to start UDP listener:", err)
		return
	}

	log.Println("[discovery] UDP listener started on port 5555")

	go func() {
		defer conn.Close()
		buf := make([]byte, 1024)

		for {
			n, remoteAddr, err := conn.ReadFromUDP(buf)
			if err != nil {
				log.Println("[discovery] UDP read error:", err)
				continue
			}

			msg := string(buf[:n])
			if msg == "DERP_DISCOVER_REQUEST" {
				response := "DERP_MASTER_IDENTITY|" + serverName
				_, err := conn.WriteToUDP([]byte(response), remoteAddr)
				if err != nil {
					log.Println("[discovery] UDP write error:", err)
				} else {
					log.Println("[discovery] Responded to discovery request from", remoteAddr.IP)
				}
			}
		}
	}()
}

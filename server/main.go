package main

import (
	"bufio"
	"encoding/binary"
	"log"
	"net"
	"sync"
	"time"
)

var clientMutex = sync.Mutex{}
var clientCtr = 0
var clients = make(map[int]*net.Conn)

func addClient(conn *net.Conn) int {
	clientMutex.Lock()
	defer clientMutex.Unlock()
	id := clientCtr
	clientCtr += 1
	clients[id] = conn
	log.Println("Connected client", id, "from", (*conn).RemoteAddr())
	return id
}

func removeClient(id int) {
	clientMutex.Lock()
	defer clientMutex.Unlock()
	clients[id] = nil
	log.Println("Lost connection to client", id)
}

func broadcastMessage(data []byte) {
	clientMutex.Lock()
	defer clientMutex.Unlock()
	for _, v := range clients {
		_, err := (*v).Write(data)
		if err != nil {
			log.Println("Failed to send", err)
			continue
		}
	}
}

func handleConnection(c net.Conn) {
	defer c.Close()

	for {
		netData, err := bufio.NewReader(c).ReadString('\n')
		if err != nil {
			log.Println("Connection failure", err.Error())
			return
		}

		if netData == "LOGIN DVTP/0.1\n" {
			id := addClient(&c)
			defer removeClient(id)
		}

		println(netData)
	}
}

func acceptClients() {
	l, err := net.Listen("tcp", ":34221")
	if err != nil {
		log.Fatalln("Failed to start listener", err)
	}

	log.Println("Server started")

	for {
		c, err := l.Accept()
		if err != nil {
			log.Fatalln("Failed to accept client", err)
		}
		go handleConnection(c)
	}
}

func rgb15(r int, g int, b int) uint16 {
	return uint16(((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3) | (1 << 15))
}

func putPixel(dst []byte, x int, y int, r int, g int, b int) {
	binary.LittleEndian.PutUint16(dst[y*256+x:], rgb15(r, g, b))
}

func main() {
	go acceptClients()

	// Real deal here
	framebuffer := make([]uint8, 256*256*2)

	ticker := time.NewTicker(500 * time.Millisecond)
	for range ticker.C {
		for y := 60; y < 120; y++ {
			for x := 60; x < 120; x++ {
				putPixel(framebuffer, x, y, 5, 25, 255)
			}
		}
		broadcastMessage(framebuffer)
	}

}

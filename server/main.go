package main

import (
	"bufio"
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

		if netData == "LOGIN DVTP/0.1" {
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

func putPixel(dst []byte, x int, y int, r int, g int, b int) {

}

func main() {
	go acceptClients()

	// Real deal here
	framebuffer := make([]uint8, 256*256*2)

	timer := time.NewTimer(500 * time.Millisecond)
	select {
	case <-timer.C:
		for y := 60; y < 120; y++ {
			for x := 60; x < 120; x++ {
				putPixel(framebuffer, x, y, 255, 255, 255)
			}
		}
		broadcastMessage(framebuffer)
	}
}

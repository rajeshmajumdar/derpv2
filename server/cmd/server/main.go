package main

import (
	"log"
	"net/http"
	"os"

	"derp-server/internal/database"
	"derp-server/internal/handlers"
)

var jwtKey = []byte("derp_secret_key_2026") //TODO: Move to .env

func main() {
	db, err := database.InitDB("./derp.db")
	if err != nil {
		log.Fatal("Critical db component crash trace: ", err)
	}
	defer db.Close()

	hostname, _ := os.Hostname()
	handlers.StartDiscoveryListener(hostname)

	http.HandleFunc("/api/v1/setup", handlers.SetupHandler(db))
	http.HandleFunc("/api/v1/login", handlers.LoginHandler(db, jwtKey))
	http.HandleFunc("/api/v1/ping", handlers.PingHandler())
	http.HandleFunc("/api/v1/admin/staff", handlers.StaffHandler(db))
	http.HandleFunc("/api/v1/admin/permissions", handlers.PermissionHandler(db))
	http.HandleFunc("/api/v1/user/permissions", handlers.GetPermissionsHandler(db, jwtKey))
	http.HandleFunc("/api/v1/system/intents", handlers.IntentsHandler(db))

	log.Println("[+] server running at 0.0.0.0:5000")
	if err := http.ListenAndServe("0.0.0.0:5000", nil); err != nil {
		log.Fatal("Port transport lock error: ", err)
	}
}

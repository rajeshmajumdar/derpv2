package handlers

import (
	"database/sql"
	"encoding/json"
	"log"
	"net/http"
)

type IntentPayload struct {
	ProtectedIntents map[string][]string `json:"protected_intents"`
}

func IntentsHandler(db *sql.DB) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		switch r.Method {
		case http.MethodPost:
			handlePostIntents(db, w, r)
		case http.MethodGet:
			handleGetIntents(db, w, r)
		default:
			http.Error(w, `{"error": "Method not allowed."}`, http.StatusMethodNotAllowed)
		}
	}
}


func handlePostIntents(db *sql.DB, w http.ResponseWriter, r *http.Request) {
	var payload IntentPayload
	if err := json.NewDecoder(r.Body).Decode(&payload); err != nil {
		http.Error(w, `{"error": "Invalid JSON payload"}`, http.StatusBadRequest)
		return
	}

	stmt, err := db.Prepare("INSERT OR IGNORE INTO Protected_Intents (plugin_name, permission_name) VALUES (?, ?)")
	if err != nil {
		log.Println("[Server] DB prepare error:", err)
		http.Error(w, `{"error": "Database error"}`, http.StatusInternalServerError)
		return
	}
	defer stmt.Close()

	//TODO: get this from .env
	const adminID = "admin"

	insertedCount := 0

	for pluginName, permissions := range payload.ProtectedIntents {
		for _, perm := range permissions {
			res, err := stmt.Exec(pluginName, perm)
			if err != nil {
				log.Printf("[Server] Failed to insert %s.%s: %v\n", pluginName, perm)
				continue
			}

			db.Exec(
				"INSERT OR IGNORE INTO Staff_Permissions (staff_id, plugin_name, permission_name) VALUES (?, ?, ?)",
				adminID, pluginName, perm,
			)

			rowsAffected, _ := res.RowsAffected()
			if rowsAffected > 0 {
				insertedCount++
			}
		}
	}

	log.Printf("[Server] Synced intents. %d new intents added to the table", insertedCount)

	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"status": "success"})
}


func handleGetIntents(db *sql.DB, w http.ResponseWriter, r *http.Request) {
	rows, err := db.Query("SELECT plugin_name, permission_name FROM Protected_Intents")
	if err != nil {
		log.Println("[Server] DB Query error:", err)
		http.Error(w, `{"error": "Failed to fetch intents"}`, http.StatusInternalServerError)
		return
	}
	defer rows.Close()

	groupedIntents := make(map[string][]string)

	for rows.Next() {
		var pluginName, permName string
		if err := rows.Scan(&pluginName, &permName); err != nil {
			continue
		}

		groupedIntents[pluginName] = append(groupedIntents[pluginName], permName)
	}

	response := IntentPayload{
		ProtectedIntents: groupedIntents,
	}

	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(response)
}

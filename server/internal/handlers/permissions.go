package handlers

import (
	"database/sql"
	"encoding/json"
	"net/http"
)

type PermissionToggleRequest struct {
	StaffID			string	`json:"staff_id"`
	Plugin			string	`json:"plugin"`
	Permission	string	`json:"permission"`
	Allowed			bool		`json:"allowed"`
}

type BulkPermissionRequest struct {
	Updates []PermissionToggleRequest `json:"updates"`
}

func PermissionHandler(db *sql.DB) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		switch r.Method {
		case http.MethodPost:
			togglePermissionHandler(db, w, r)
		case http.MethodGet:
			getStaffPermissionsHandler(db, w, r)
		default:
			http.Error(w, `{"error": "Method not allowed."}`, http.StatusMethodNotAllowed)
		}
	}
}


func getStaffPermissionsHandler(db *sql.DB, w http.ResponseWriter, r *http.Request) {
	staffID := r.URL.Query().Get("staff_id")
	if staffID == "" {
		http.Error(w, `{"error": "id parameter is required"}`, http.StatusBadRequest)
		return
	}

	rows, err := db.Query("SELECT plugin_name, permission_name FROM Staff_Permissions WHERE staff_id = ?", staffID)
	if err != nil {
		http.Error(w, `{"error": "Database error"}`, http.StatusInternalServerError)
		return
	}
	defer rows.Close()

	allowedIntents := make(map[string][]string)
	for rows.Next() {
		var plugin, perm string
		if err := rows.Scan(&plugin, &perm); err == nil {
			allowedIntents[plugin] = append(allowedIntents[plugin], perm)
		}
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(allowedIntents)
}


func togglePermissionHandler(db *sql.DB, w http.ResponseWriter, r *http.Request) {
	var req BulkPermissionRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, `{"error": "Invalid payload"}`, http.StatusBadRequest)
		return
	}

	tx, err := db.Begin()
	if err != nil {
		http.Error(w, `{"error": "Database error"}`, http.StatusInternalServerError)
		return
	}

	insertStmt, _ := tx.Prepare("INSERT OR IGNORE INTO Staff_Permissions (staff_id, plugin_name, permission_name) VALUES (?, ?, ?)")
	deleteStmt, _ := tx.Prepare("DELETE FROM Staff_Permissions WHERE staff_id = ? AND plugin_name = ? AND permission_name = ?")

	defer insertStmt.Close()
	defer deleteStmt.Close()

	for _, update := range req.Updates {
		if update.StaffID == "admin" && !update.Allowed {
			continue
		}

		if update.Allowed {
			insertStmt.Exec(update.StaffID, update.Plugin, update.Permission)
		} else {
			deleteStmt.Exec(update.StaffID, update.Plugin, update.Permission)
		}
	}

	tx.Commit();

	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"status": "success"})
}

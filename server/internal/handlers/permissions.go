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
	var req PermissionToggleRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, `{"error": "Invalid payload"}`, http.StatusBadRequest)
		return
	}

	if req.StaffID == "admin" && !req.Allowed {
		http.Error(w, `{"error": "Cannot revoke admin permissions"}`, http.StatusForbidden)
		return
	}

	var err error
	if req.Allowed {
		_, err = db.Exec(
			"INSERT OR IGNORE INTO Staff_Permissions (staff_id, plugin_name, permission_name) VALUES (?, ?, ?)",
			req.StaffID, req.Plugin, req.Permission,
		)
	} else {
		_, err = db.Exec(
			"DELETE FROM Staff_Permissions WHERE staff_id = ? AND plugin_name = ? AND permission_name = ?",
			req.StaffID, req.Plugin, req.Permission,
		)
	}

	if err != nil {
		http.Error(w, `{"error": "Database error"}`, http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"status": "success"})
}

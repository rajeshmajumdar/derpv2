package handlers

import (
	"database/sql"
	"encoding/json"
	"net/http"

	"fmt"

	"derp-server/internal/models"
	"golang.org/x/crypto/bcrypt"
)

func isAdmin(r *http.Request) bool {
	// TODO: We need to do JWT vertification here
	return true
}

func StaffHandler(db *sql.DB) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		if !isAdmin(r) {
			http.Error(w, `{"error": "Unauthorized access"}`, http.StatusForbidden)
			return
		}

		switch r.Method {
		case http.MethodGet:
			// List all non-master users
			rows, err := db.Query("SELECT id, username, role FROM users WHERE role != 'master'")
			if err != nil {
				w.WriteHeader(http.StatusInternalServerError)
				// TODO: Diagnostics
				fmt.Println(err)
				return
			}
			defer rows.Close()

			var staffList []models.StaffUser
			for rows.Next() {
				var u models.StaffUser
				if err := rows.Scan(&u.ID, &u.Username, &u.Role); err == nil {
					staffList = append(staffList, u)
				}
			}
			json.NewEncoder(w).Encode(staffList)

			
		case http.MethodPost:
			var req models.StaffUser
			if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
				w.WriteHeader(http.StatusBadRequest)
				return
			}

			if req.Password != "" {
				hash, _ := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
				_, err := db.Exec("INSERT INTO users (username, password_hash, role) VALUES (?, ?, 'staff')",
									req.Username, string(hash))
				if err != nil {
					http.Error(w, `{"error": "Username already exists"}`, http.StatusConflict)
					return
				}
			}

			w.WriteHeader(http.StatusOK)
			json.NewEncoder(w).Encode(map[string]string{"status": "success"})


		default:
			w.WriteHeader(http.StatusMethodNotAllowed)
		}
	}
}

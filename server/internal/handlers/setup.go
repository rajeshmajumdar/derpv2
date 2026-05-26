package handlers

import (
	"database/sql"
	"encoding/json"
	"log"
	"net/http"

	"derp-server/internal/models"
	"golang.org/x/crypto/bcrypt"
)

func SetupHandler(db *sql.DB) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		var payload models.SetupPayload
		if err := json.NewDecoder(r.Body).Decode(&payload); err != nil {
			http.Error(w, "Invalid configuration vector data", http.StatusBadRequest)
			return
		}

		tx, err := db.Begin()
		if err != nil {
			http.Error(w, "Transaction context mount failure", http.StatusInternalServerError)
			return
		}

		compQuery := `
		INSERT OR REPLACE INTO company (id, name, tagline, address, contact, email, website, dl_number, gstin, fssai, iec_code)
		VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`
		_, err = tx.Exec(compQuery,
			payload.Company.Name, payload.Company.Tagline, payload.Company.Address, 
			payload.Company.Contact, payload.Company.Email, payload.Company.Website,
			payload.Company.DlNumber, payload.Company.Gstin, payload.Company.Fssai,
			payload.Company.IecCode,
		)
		if err != nil {
			tx.Rollback()
			log.Println("Company persistence drop error:", err)
			http.Error(w, "Company table write error", http.StatusInternalServerError)
			return
		}

		hashedPass, err := bcrypt.GenerateFromPassword([]byte(payload.Admin.Password), bcrypt.DefaultCost)
		if err != nil {
			tx.Rollback()
			http.Error(w, "Cryptographic runtime error", http.StatusInternalServerError)
			return
		}

		userQuery := `INSERT OR REPLACE INTO users (username, password_hash, role) VALUES (?, ?, 'master')`
		if _, err = tx.Exec(userQuery, payload.Admin.Username, string(hashedPass)); err != nil {
			tx.Rollback()
			log.Println("Admin persistence drop error:", err)
			http.Error(w, "User table write error", http.StatusInternalServerError)
			return
		}

		if err := tx.Commit(); err != nil {
			http.Error(w, "Database sync execution failed", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]string{"status": "success"})
	}
}

package handlers

import (
	"database/sql"
	"encoding/json"
	"net/http"
	"time"

	"derp-server/internal/models"
	"github.com/golang-jwt/jwt/v5"
	"golang.org/x/crypto/bcrypt"
)

func LoginHandler(db *sql.DB, jwtKey []byte) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		var creds models.LoginCredentials
		if err := json.NewDecoder(r.Body).Decode(&creds); err != nil {
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		var passHash string
		var role string

		query := "SELECT password_hash, role FROM users WHERE username = ?"
		err := db.QueryRow(query, creds.Username).Scan(&passHash, &role)

		w.Header().Set("Content-Type", "application/json")
		if err != nil || bcrypt.CompareHashAndPassword([]byte(passHash), []byte(creds.Password)) != nil {
			w.WriteHeader(http.StatusUnauthorized)
			json.NewEncoder(w).Encode(map[string]string{"message": "Invalid credentials"})
			return
		}

		expirationTime := time.Now().Add(12 * time.Hour)
		claims := &models.Claims{
			Username: creds.Username,
			Role:			role,
			RegisteredClaims: jwt.RegisteredClaims{
				ExpiresAt: jwt.NewNumericDate(expirationTime),
			},
		}

		token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
		tokenString, err := token.SignedString(jwtKey)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(map[string]string{
			"token": tokenString,
			"role": role,
			"user": creds.Username,
		})
	}
}

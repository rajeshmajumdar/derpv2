package handlers

import (
	"database/sql"
	"encoding/json"
	"net/http"
	"strings"

	"derp-server/internal/models"
	"github.com/golang-jwt/jwt/v5"
)

func GetPermissionsHandler(db *sql.DB, jwtKey []byte) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, `{"error": "Method not allowed"}`, http.StatusMethodNotAllowed)
			return
		}

		authHeader := r.Header.Get("Authorization")
		if !strings.HasPrefix(authHeader, "Bearer ") {
			http.Error(w, `{"error": "Unauthorized"}`, http.StatusUnauthorized)
			return
		}

		tokenString := strings.TrimPrefix(authHeader, "Bearer ")
		claims := &models.Claims{}
		token, err := jwt.ParseWithClaims(tokenString, claims, func(token *jwt.Token) (interface{}, error) {
			return jwtKey, nil
		})

		if err != nil || !token.Valid {
			http.Error(w, `{"error": "Invalid or expired token"}`, http.StatusUnauthorized)
			return
		}

		staffID := claims.Username

		rows, err := db.Query("SELECT plugin_name, permission_name FROM Staff_Permissions WHERE staff_id = ?", staffID)
		if err != nil {
			http.Error(w, `{"error": "Database error"}`, http.StatusInternalServerError)
			return
		}
		defer rows.Close()

		userPermissions := make(map[string][]string)
		for rows.Next() {
			var plugin, perm string
			if err := rows.Scan(&plugin, &perm); err == nil {
				userPermissions[plugin] = append(userPermissions[plugin], perm)
			}
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"permissions": userPermissions,
		})
	}
}

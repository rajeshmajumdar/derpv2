package database

import (
	"database/sql"

	_ "github.com/mattn/go-sqlite3"
)

func InitDB(sourceName string) (*sql.DB, error) {
	db, err := sql.Open("sqlite3", sourceName)
	if err != nil {
		return nil, err
	}

	userTable := `
	CREATE TABLE IF NOT EXISTS users (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		username TEXT UNIQUE,
		password_hash TEXT,
		role TEXT
	);
	`
	if _, err := db.Exec(userTable); err != nil {
		return nil, err
	}

	companyTable := `
	CREATE TABLE IF NOT EXISTS company (
		id INTEGER PRIMARY KEY CHECK (id = 1),
		name TEXT,
		tagline TEXT,
		address TEXT,
		contact TEXT,
		email TEXT,
		website TEXT,
		dl_number TEXT,
		gstin TEXT,
		fssai TEXT,
		iec_code TEXT
	);`

	if _, err := db.Exec(companyTable); err != nil {
		return nil, err
	}

	protectedIntentsTable := `
	CREATE TABLE IF NOT EXISTS Protected_Intents (
		plugin_name TEXT NOT NULL,
		permission_name TEXT NOT NULL,
		UNIQUE(plugin_name, permission_name)
	);`

	if _, err := db.Exec(protectedIntentsTable); err != nil {
		return nil, err
	}

	staffPermissionsTable := `
	CREATE TABLE IF NOT EXISTS Staff_Permissions (
		staff_id TEXT NOT NULL,
		plugin_name TEXT NOT NULL,
		permission_name TEXT NOT NULL,
		UNIQUE(staff_id, plugin_name, permission_name)
	);`

	if _, err := db.Exec(staffPermissionsTable); err != nil {
		return nil, err
	}

	return db, nil
}

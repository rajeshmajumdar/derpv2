package models

import "github.com/golang-jwt/jwt/v5"

type CompanyProfile struct {
	Name			string `json:"name"`
	Tagline		string `json:"tagline"`
	Address		string `json:"address"`
	Contact		string `json:"contact"`
	Email			string `json:"email"`
	Website		string `json:"website"`
	DlNumber	string `json:"dl_number"`
	Gstin			string `json:"gstin"`
	Fssai			string `json:"fssai"`
	IecCode		string `json:"iec_code"`
}

type AdminAccount struct {
	Username	string `json:"username"`
	Password	string `json:"password"`
}

type SetupPayload struct {
	Company		CompanyProfile	`json:"company"`
	Admin			AdminAccount		`json:"admin"`
}

type LoginCredentials struct {
	Username		string `json:"username"`
	Password		string `json:"password"`
}

type Claims struct {
	Username		string `json:"username"`
	Role				string `json:"role"`
	jwt.RegisteredClaims
}

type StaffUser struct {
	ID					int			`json:"id"`
	Username		string 	`json:"username"`
	Password		string 	`json:"password,omitempty"`
	Role				string	`json:"role"`
}

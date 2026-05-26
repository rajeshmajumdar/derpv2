🛠️ Phase 1: Core Infrastructure (C++)
[x] Finalize SetupWizard UI: Create a polished QDialog that saves the selected role and the Master IP (for staff) into QSettings.

[x] Implement mDNS Discovery: Add a discovery service (like QLDNSServiceBrowser) so Staff terminals can find the Master PC without manual IP entry.

[x] Strengthen QProcess Management: Ensure server.exe is gracefully terminated when the Kernel closes, and handle "Process Failed to Start" scenarios with a notification.

[x] Update Plugin Manifests: Add the "role": "master" key to all administrative plugins to test the filtering logic in the Kernel.

🚀 Phase 2: Go Backend (The server.exe)
[ ] Project Scaffolding: Initialize the Go module and set up the directory structure (/cmd, /internal/api, /internal/db).

[ ] SQLite Initializer: Write the logic to create derp.db and the required tables (users, roles, permissions) if the file doesn't exist.

[ ] mDNS Advertising: Implement the advertiser logic in Go so the server broadcasts its presence on the local network.

[ ] Basic Auth API: Create the /api/v1/login and /api/v1/health endpoints to allow the C++ app to verify the connection.

📦 Phase 3: Master-Only Plugins
[ ] Server Monitor Plugin: Build a UI that displays real-time logs from the server.exe and shows "Heartbeat" stats (RAM usage, uptime).

[ ] Staff Management Plugin: Create the interface for the Owner to add new "Retail" or "Wholesale" counter users and assign passwords.

[ ] Database Maintenance: A simple tool for the Master to trigger a manual backup of the derp.db file to a specified directory.

📋 Phase 4: Security & Validation
[ ] Hardware Fingerprinting: Implement the basic HWID logic in the Go server to "lock" the database to the Master PC.

[ ] JWT Implementation: Ensure the Go server issues tokens that include the user's role and specific permission strings.

[ ] Network Firewall Check: Add a check to see if the Windows Firewall is blocking port 5000 and prompt the Master to allow it.

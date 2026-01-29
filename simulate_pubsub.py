#!/usr/bin/env python3
"""
Pub-Sub System Simulator - Busy Server Mode
Opens multiple terminal windows and simulates automated message traffic
"""

import subprocess
import time
import sys
import random

def open_terminal_with_command(title, command, position=None):
    """
    Opens a new terminal window with the given command using osascript (macOS)
    """
    # AppleScript to open new Terminal window
    script = f'''
    tell application "Terminal"
        activate
        do script "{command}"
        set custom title of front window to "{title}"
    end tell
    '''
    
    try:
        subprocess.run(['osascript', '-e', script], check=True)
        print(f"✓ Opened terminal: {title}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"✗ Failed to open terminal: {title}")
        print(f"  Error: {e}")
        return False

def create_publisher_script(script_dir, publisher_id, num_messages=10, delay_range=(1, 3)):
    """
    Creates a shell script that automatically sends messages from a publisher
    """
    messages = [
        f"Product update from Publisher {publisher_id}",
        f"Alert: System status from Publisher {publisher_id}",
        f"News: Breaking update #{publisher_id}",
        f"Notification: User activity detected by Publisher {publisher_id}",
        f"Update: Metrics report from Publisher {publisher_id}",
        f"Info: Service health check from Publisher {publisher_id}",
        f"Data: Analytics from Publisher {publisher_id}",
        f"Event: New transaction logged by Publisher {publisher_id}",
        f"Status: All systems operational - Publisher {publisher_id}",
        f"Message: Broadcast #{publisher_id} to all subscribers"
    ]
    
    script_content = f"""#!/bin/bash
cd {script_dir}

# Function to send messages automatically
send_messages() {{
    sleep 3  # Wait for connection
    
    for i in {{1..{num_messages}}}; do
        MESSAGE="{messages[publisher_id % len(messages)]}"
        echo "$MESSAGE"
        sleep {random.uniform(delay_range[0], delay_range[1]):.1f}
    done
    
    echo "terminate"
}}

# Connect and start sending
send_messages | ./my_client_app 127.0.0.1 8080 PUBLISHER
"""
    
    script_path = f"{script_dir}/publisher_{publisher_id}_auto.sh"
    with open(script_path, 'w') as f:
        f.write(script_content)
    
    subprocess.run(['chmod', '+x', script_path])
    return script_path

def main():
    print("=" * 70)
    print("Pub-Sub System Simulator - BUSY SERVER MODE")
    print("=" * 70)
    print("\nThis script will simulate a busy server with:")
    print("  - 1 Server")
    print("  - 3 Subscribers (receiving messages)")
    print("  - 5 Publishers (auto-sending messages)")
    print("\nPublishers will automatically send messages every 1-3 seconds")
    print("to simulate real-world traffic.")
    print("=" * 70)
    
    # Get the directory where this script is located
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Configuration
    num_publishers = 5
    num_subscribers = 3
    messages_per_publisher = 15
    
    # Confirm with user
    print(f"\nConfiguration:")
    print(f"  Publishers: {num_publishers}")
    print(f"  Subscribers: {num_subscribers}")
    print(f"  Messages per publisher: {messages_per_publisher}")
    print(f"  Estimated runtime: ~30-45 seconds")
    
    response = input("\nReady to start simulation? (y/n): ")
    if response.lower() != 'y':
        print("Cancelled.")
        return
    
    print("\nStarting busy server simulation...\n")
    
    # 1. Start Server
    print("Step 1: Starting server...")
    server_cmd = f"cd {script_dir} && ./my_server_app 8080"
    open_terminal_with_command("🖥️  Server - Port 8080", server_cmd)
    time.sleep(2)
    
    # 2. Start Subscribers
    print(f"\nStep 2: Starting {num_subscribers} subscribers...")
    for i in range(num_subscribers):
        sub_cmd = f"cd {script_dir} && ./my_client_app 127.0.0.1 8080 SUBSCRIBER"
        open_terminal_with_command(f"📥 Subscriber {i+1}", sub_cmd)
        time.sleep(0.5)
    
    # 3. Create and start Publishers
    print(f"\nStep 3: Creating automated publishers...")
    publisher_scripts = []
    for i in range(num_publishers):
        script_path = create_publisher_script(
            script_dir, 
            i, 
            num_messages=messages_per_publisher,
            delay_range=(1, 3)
        )
        publisher_scripts.append(script_path)
        print(f"  ✓ Created publisher script {i+1}")
    
    time.sleep(1)
    
    print(f"\nStep 4: Starting {num_publishers} automated publishers...")
    for i, script_path in enumerate(publisher_scripts):
        pub_cmd = f"cd {script_dir} && {script_path}"
        open_terminal_with_command(f"📤 Publisher {i+1} (Auto)", pub_cmd)
        time.sleep(0.5)
    
    print("\n" + "=" * 70)
    print("✓ Busy server simulation started!")
    print("=" * 70)
    print("\nWhat's happening:")
    print("  • 5 publishers are automatically sending messages")
    print("  • 3 subscribers are receiving all published messages")
    print("  • Messages are sent every 1-3 seconds")
    print("  • Each publisher will send 15 messages then disconnect")
    print("\nServer window commands:")
    print("  • Type 'show_users' to see all active clients")
    print("  • Type 'help' for more commands")
    print("\nThe simulation will run for ~30-45 seconds")
    print("\nTo manually stop everything:")
    print("  pkill -f my_server_app && pkill -f my_client_app")
    print("=" * 70)
    
    # Cleanup scripts after simulation
    print("\nPress Ctrl+C when you want to cleanup the auto-scripts...")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n\nCleaning up temporary scripts...")
        for script in publisher_scripts:
            try:
                subprocess.run(['rm', script], check=False)
                print(f"  ✓ Removed {script}")
            except:
                pass
        print("Cleanup complete!")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nSimulation cancelled by user.")
        sys.exit(0)

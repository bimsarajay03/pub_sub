#!/usr/bin/env python3
"""
Pub-Sub System Simulator - Topic-Based Pub-Sub
Opens 5 terminal windows and simulates topic-based message traffic
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

def create_publisher_script(script_dir, publisher_id, topic, num_messages=10, delay_range=(3, 6)):
    """
    Creates a shell script that automatically sends messages from a publisher on a specific topic
    """
    topic_messages = {
        "sports": [
            f"Goal scored in the match!",
            f"NBA Finals update",
            f"Tennis championship result",
            f"Baseball game highlights",
            f"NFL touchdown alert"
        ],
        "news": [
            f"Breaking: Major announcement",
            f"World news update",
            f"Business market trends",
            f"Science breakthrough",
            f"Entertainment news"
        ],
        "weather": [
            f"Sunny day ahead",
            f"Rain expected tonight",
            f"Snow warning issued",
            f"Temperature rising",
            f"Storm approaching"
        ]
    }
    
    messages = topic_messages.get(topic, [f"Update from {topic} topic"])
    
    script_content = f"""#!/bin/bash
cd {script_dir}

# Function to send messages automatically
send_messages() {{
    sleep 3  # Wait for connection
    
    for i in {{1..{num_messages}}}; do
        MESSAGE="{random.choice(messages)}"
        echo "$MESSAGE"
        sleep {random.uniform(delay_range[0], delay_range[1]):.1f}
    done
    
    echo "terminate"
}}

# Connect and start sending
send_messages | ./my_client_app 127.0.0.1 8080 PUBLISHER {topic}
"""
    
    script_path = f"{script_dir}/publisher_{topic}_{publisher_id}_auto.sh"
    with open(script_path, 'w') as f:
        f.write(script_content)
    
    subprocess.run(['chmod', '+x', script_path])
    return script_path

def main():
    print("=" * 70)
    print("Pub-Sub System Simulator - TOPIC-BASED MESSAGING")
    print("=" * 70)
    print("\nThis script will simulate a topic-based pub-sub system with:")
    print("  - 1 Server")
    print("  - 2 Publishers (sports, news)")
    print("  - 2 Subscribers (sports, news)")
    print("\nPublishers will send messages every 3-6 seconds on their topics")
    print("Subscribers will only receive messages from their subscribed topics")
    print("=" * 70)
    
    # Get the directory where this script is located
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Configuration
    messages_per_publisher = 15
    
    # Confirm with user
    print(f"\nConfiguration:")
    print(f"  Publishers: 2 (sports, news)")
    print(f"  Subscribers: 2 (sports, news)")
    print(f"  Messages per publisher: {messages_per_publisher}")
    print(f"  Message delay: 3-6 seconds")
    print(f"  Estimated runtime: ~1.5 minutes")
    
    response = input("\nReady to start simulation? (y/n): ")
    if response.lower() != 'y':
        print("Cancelled.")
        return
    
    print("\nStarting topic-based simulation...\n")
    
    # 1. Start Server
    print("Step 1: Starting server...")
    server_cmd = f"cd {script_dir} && ./my_server_app 8080"
    open_terminal_with_command("Server - Port 8080", server_cmd)
    time.sleep(2)
    
    # 2. Start Subscribers on matching topics
    print(f"\nStep 2: Starting subscribers...")
    subscriber_topics = ["sports", "news"]
    for i, topic in enumerate(subscriber_topics):
        sub_cmd = f"cd {script_dir} && ./my_client_app 127.0.0.1 8080 SUBSCRIBER {topic}"
        open_terminal_with_command(f"Subscriber - {topic.upper()}", sub_cmd)
        time.sleep(1)
    
    # 3. Create and start Publishers on different topics
    print(f"\nStep 3: Creating automated publishers...")
    publisher_topics = ["sports", "news"]
    publisher_scripts = []
    
    for i, topic in enumerate(publisher_topics):
        script_path = create_publisher_script(
            script_dir, 
            i, 
            topic,
            num_messages=messages_per_publisher,
            delay_range=(3, 6)
        )
        publisher_scripts.append(script_path)
        print(f"  ✓ Created {topic} publisher script")
    
    time.sleep(1)
    
    print(f"\nStep 4: Starting automated publishers...")
    for i, (script_path, topic) in enumerate(zip(publisher_scripts, publisher_topics)):
        pub_cmd = f"cd {script_dir} && {script_path}"
        open_terminal_with_command(f"Publisher - {topic.upper()} (Auto)", pub_cmd)
        time.sleep(1)
    
    print("\n" + "=" * 70)
    print("✓ Topic-based simulation started!")
    print("=" * 70)
    print("\nWhat's happening:")
    print("  • Sports Publisher → sends to Sports Subscriber only")
    print("  • News Publisher → sends to News Subscriber only")
    print("  • Messages are isolated by topic (sports/news)")
    print("  • Messages sent every 3-6 seconds")
    print("  • Each publisher sends 15 messages then disconnects")
    print("\nServer window commands:")
    print("  • Type 'show_users' to see all active clients and their topics")
    print("  • Type 'show_topics' to see all active topics")
    print("  • Type 'help' for more commands")
    print("\nThe simulation will run for ~1.5 minutes")
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

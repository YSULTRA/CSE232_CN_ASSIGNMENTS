import subprocess
import re

def run_traceroute(host):
    try:
        result = subprocess.run(['traceroute', host], capture_output=True, text=True, check=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error running traceroute: {e}")
        return None

def parse_traceroute(output):
    intermediate_hosts = []
    lines = output.strip().split('\n')[1:]

    for line in lines:
        parts = line.split()
        if len(parts) > 2:
            ip_addresses = re.findall(r'\((.*?)\)', line)
            latencies = re.findall(r'(\d+\.\d+)\sms', line)

            if ip_addresses:
                ip_address = ip_addresses[0]
                latencies = list(map(float, latencies))

                if latencies:
                    avg_latency = sum(latencies) / len(latencies)
                    intermediate_hosts.append((ip_address, avg_latency))

    return intermediate_hosts

def display_results(intermediate_hosts, file):
    num_hosts = len(intermediate_hosts)
    results = [f"Number of intermediate hosts: {num_hosts}"]

    for index, (ip, latency) in enumerate(intermediate_hosts, start=1):
        results.append(f"Hop {index}: IP = {ip}, Avg Latency = {latency:.2f} ms")

    print('\n'.join(results))
    with open(file, 'w') as f:
        f.write("\n".join(results))

def main():
    host = 'google.in'
    traceroute_output = run_traceroute(host)

    if traceroute_output:
        with open('traceroute_output.txt', 'w') as file:
            file.write(traceroute_output)
            print("\nTraceroute output saved to 'traceroute_output.txt'. Results saved to 'results_output.txt'.")

        intermediate_hosts = parse_traceroute(traceroute_output)
        display_results(intermediate_hosts, 'results_output.txt')
    else:
        print("Failed to retrieve traceroute output.")

if __name__ == "__main__":
    main()

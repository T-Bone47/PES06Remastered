#ifndef REMASTER_TELEMETRY_H
#define REMASTER_TELEMETRY_H

namespace remaster {
namespace telemetry {

// Starts the read-only match telemetry thread. It does nothing unless
// "telemetry enabled" is set in the config.
bool install();

// Stops the thread.
void shutdown();

}  // namespace telemetry
}  // namespace remaster

#endif  // REMASTER_TELEMETRY_H

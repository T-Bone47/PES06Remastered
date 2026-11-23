#ifndef REMASTER_IO_FILEIO_H
#define REMASTER_IO_FILEIO_H

namespace remaster {
namespace fileio {

// Installs the file-I/O logger. It only observes: every call is forwarded
// unchanged. Returns false if the hooks could not be installed; the game is
// unaffected in that case.
bool install();

void shutdown();

// Polls the save folder and reports any file that appeared or changed.
// Called from the telemetry thread; does nothing unless the logger is on.
void poll();

}  // namespace fileio
}  // namespace remaster

#endif  // REMASTER_IO_FILEIO_H


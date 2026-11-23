//DecompileAddresses
//Decompiles an explicit list of addresses from an already-analysed project.
//Usage: -postScript DecompileAddresses <addrs.txt> <out.txt>
//@category Analysis
//@author rea

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.List;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class DecompileAddresses extends GhidraScript {

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 2) {
			printerr("usage: DecompileAddresses <addrs.txt> <out.txt>");
			return;
		}
		List<String> lines = java.nio.file.Files.readAllLines(new File(args[0]).toPath());

		DecompInterface decomp = new DecompInterface();
		DecompileOptions opts = new DecompileOptions();
		decomp.setOptions(opts);
		decomp.openProgram(currentProgram);

		PrintWriter out = new PrintWriter(new OutputStreamWriter(
			new FileOutputStream(args[1]), StandardCharsets.UTF_8));
		out.println("=== program: " + currentProgram.getName() + " ===");

		for (String raw : lines) {
			String t = raw.trim();
			if (t.isEmpty() || t.startsWith("#")) {
				continue;
			}
			int sp = t.indexOf(' ');
			String addrHex = (sp < 0) ? t : t.substring(0, sp);
			String note = (sp < 0) ? "" : t.substring(sp + 1).trim();
			long va = Long.parseLong(addrHex.replace("0x", ""), 16);
			Address addr = currentProgram.getAddressFactory().getDefaultAddressSpace()
					.getAddress(va);

			out.println();
			out.println("################ " + addrHex + "  " + note + " ################");
			Function fn = currentProgram.getFunctionManager().getFunctionContaining(addr);
			Function exact = currentProgram.getFunctionManager().getFunctionAt(addr);
			if (fn == null) {
				out.println("(no function contains this address)");
				out.flush();
				continue;
			}
			out.println("// containing function: " + fn.getName() + " @ " + fn.getEntryPoint());
			out.println("// signature: " + fn.getSignature().toString());
			out.println("// exact entry: " + (exact != null));
			try {
				DecompileResults res = decomp.decompileFunction(fn, 180, monitor);
				if (res != null && res.decompileCompleted()) {
					out.println(res.getDecompiledFunction().getC());
				}
				else {
					out.println("(decompile failed: "
						+ (res == null ? "null" : res.getErrorMessage()) + ")");
				}
			}
			catch (Exception e) {
				out.println("(exception: " + e.getMessage() + ")");
			}
			out.flush();
			println("decompiled " + addrHex + " -> " + fn.getName());
		}
		out.close();
		decomp.dispose();
		println("wrote " + args[1]);
	}
}

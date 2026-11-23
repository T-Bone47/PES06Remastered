//DumpFunctionListing
//Dumps the raw instruction listing (and decompilation) for functions containing
//the given addresses. Used to find exact byte patterns for runtime patching.
//Usage: -postScript DumpFunctionListing <addrs.txt> <out.txt>
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
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;

public class DumpFunctionListing extends GhidraScript {

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 2) {
			printerr("usage: DumpFunctionListing <addrs.txt> <out.txt>");
			return;
		}
		List<String> lines = java.nio.file.Files.readAllLines(new File(args[0]).toPath());

		DecompInterface decomp = new DecompInterface();
		DecompileOptions opts = new DecompileOptions();
		decomp.setOptions(opts);
		decomp.openProgram(currentProgram);

		PrintWriter out = new PrintWriter(new OutputStreamWriter(
			new FileOutputStream(args[1]), StandardCharsets.UTF_8));

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

			Function fn = currentProgram.getFunctionManager().getFunctionContaining(addr);
			out.println();
			out.println("################ " + addrHex + "  " + note + " ################");
			if (fn == null) {
				out.println("(no function)");
				out.flush();
				continue;
			}
			out.println("// function " + fn.getName() + " @ " + fn.getEntryPoint()
					+ "  size=" + fn.getBody().getNumAddresses());
			out.println();
			out.println("// ---- DECOMPILATION ----");
			try {
				DecompileResults res = decomp.decompileFunction(fn, 180, monitor);
				if (res != null && res.decompileCompleted()) {
					out.println(res.getDecompiledFunction().getC());
				}
				else {
					out.println("(decompile failed)");
				}
			}
			catch (Exception e) {
				out.println("(exception: " + e.getMessage() + ")");
			}
			out.println();
			out.println("// ---- LISTING ----");
			InstructionIterator it = currentProgram.getListing().getInstructions(fn.getBody(), true);
			while (it.hasNext()) {
				Instruction ins = it.next();
				byte[] insBytes = new byte[ins.getLength()];
				ins.getBytes(insBytes, 0);
				StringBuilder bytes = new StringBuilder();
				for (int i = 0; i < insBytes.length && i < 12; i++) {
					bytes.append(String.format("%02x", insBytes[i] & 0xff));
				}
				out.println(String.format("%s  %-20s %s",
					ins.getAddress(), bytes.toString(), ins.toString()));
			}
			out.flush();
			println("dumped " + fn.getName());
		}
		out.close();
		decomp.dispose();
		println("wrote " + args[1]);
	}
}

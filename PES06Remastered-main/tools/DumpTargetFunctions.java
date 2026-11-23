//DumpTargetFunctions
//Dumps decompilation + metadata for every PESMod target address in one headless run.
//@category Analysis
//@author rea
//@keybinding
//@menupath
//@toolbar

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;

public class DumpTargetFunctions extends GhidraScript {

	private static final int DECOMPILE_TIMEOUT_SECONDS = 180;

	private static String j(String s) {
		if (s == null) {
			return "";
		}
		StringBuilder b = new StringBuilder();
		for (int i = 0; i < s.length(); i++) {
			char c = s.charAt(i);
			switch (c) {
				case '"':
					b.append("\\\"");
					break;
				case '\\':
					b.append("\\\\");
					break;
				case '\n':
					b.append("\\n");
					break;
				case '\r':
					b.append("\\r");
					break;
				case '\t':
					b.append("\\t");
					break;
				default:
					if (c < 0x20) {
						b.append(String.format("\\u%04x", (int) c));
					}
					else {
						b.append(c);
					}
			}
		}
		return b.toString();
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 2) {
			printerr("usage: DumpTargetFunctions <targets.txt> <out.json> [functions.tsv] [strings.tsv]");
			return;
		}
		String targetFile = args[0];
		String outFile = args[1];
		String funcFile = args.length > 2 ? args[2] : null;
		String strFile = args.length > 3 ? args[3] : null;

		List<String[]> targets = new ArrayList<String[]>();
		for (String line : java.nio.file.Files.readAllLines(new File(targetFile).toPath())) {
			String t = line.trim();
			if (t.isEmpty() || t.startsWith("#")) {
				continue;
			}
			int tab = t.indexOf('\t');
			if (tab < 0) {
				tab = t.indexOf(' ');
			}
			if (tab < 0) {
				continue;
			}
			targets.add(new String[] { t.substring(0, tab).trim(), t.substring(tab + 1).trim() });
		}
		println("DumpTargetFunctions: " + targets.size() + " targets");

		DecompInterface decomp = new DecompInterface();
		try {
			DecompileOptions opts = new DecompileOptions();
			decomp.setOptions(opts);
			decomp.setSimplificationStyle("decompile");
			if (!decomp.openProgram(currentProgram)) {
				printerr("decompiler failed to open program");
				return;
			}

			PrintWriter out = new PrintWriter(new OutputStreamWriter(
				new FileOutputStream(outFile), StandardCharsets.UTF_8));
			out.println("{");
			out.println("  \"program\": \"" + j(currentProgram.getName()) + "\",");
			out.println("  \"imageBase\": \"" + j(currentProgram.getImageBase().toString()) + "\",");
			out.println("  \"targets\": [");

			for (int i = 0; i < targets.size(); i++) {
				String vaHex = targets.get(i)[0];
				String label = targets.get(i)[1];
				long va = Long.parseLong(vaHex.replace("0x", ""), 16);
				Address addr = currentProgram.getAddressFactory().getDefaultAddressSpace()
						.getAddress(va);

				StringBuilder rec = new StringBuilder();
				rec.append("    {");
				rec.append("\"va\": \"").append(vaHex).append("\", ");
				rec.append("\"label\": \"").append(j(label)).append("\"");

				Function fn = currentProgram.getFunctionManager().getFunctionContaining(addr);
				Function exact = currentProgram.getFunctionManager().getFunctionAt(addr);
				Address resolved = exact != null ? exact.getEntryPoint()
						: (fn != null ? fn.getEntryPoint() : null);

				if (fn == null) {
					rec.append(", \"status\": \"no_function\"");
					rec.append(", \"nearest\": \"").append(j(nearestBefore(addr))).append("\"");
					rec.append(", \"bytes\": \"").append(j(hexAt(addr, 16))).append("\"");
				}
				else {
					rec.append(", \"status\": \"ok\"");
					rec.append(", \"function\": \"").append(j(fn.getName())).append("\"");
					rec.append(", \"entry\": \"").append(j(fn.getEntryPoint().toString())).append("\"");
					rec.append(", \"signature\": \"").append(j(fn.getSignature().toString())).append("\"");
					rec.append(", \"size\": ").append(fn.getBody().getNumAddresses());
					rec.append(", \"exact_entry\": ").append(exact != null);
					rec.append(", \"callingConvention\": \"").append(j(fn.getCallingConventionName())).append("\"");
					rec.append(", \"nparams\": ").append(fn.getParameterCount());
					rec.append(", \"thunk\": ").append(fn.isThunk());
					rec.append(", \"external\": ").append(fn.isExternal());
					rec.append(", \"callers\": ").append(fn.getCallingFunctions(monitor).size());
					rec.append(", \"callees\": ").append(fn.getCalledFunctions(monitor).size());

					if (exact == null) {
						rec.append(", \"offset_into_function\": ")
								.append(addr.subtract(fn.getEntryPoint()));
					}

					try {
						DecompileResults res = decomp.decompileFunction(fn, DECOMPILE_TIMEOUT_SECONDS,
							monitor);
						if (res != null && res.decompileCompleted()) {
							rec.append(", \"pseudocode\": \"").append(j(res.getDecompiledFunction().getC())).append("\"");
						}
						else {
							rec.append(", \"pseudocode\": null");
							if (res != null && res.getErrorMessage() != null) {
								rec.append(", \"decompileError\": \"")
										.append(j(res.getErrorMessage())).append("\"");
							}
						}
					}
					catch (Exception e) {
						rec.append(", \"pseudocode\": null, \"decompileError\": \"")
								.append(j(String.valueOf(e.getMessage()))).append("\"");
					}
				}
				rec.append("}");
				if (i < targets.size() - 1) {
					rec.append(",");
				}
				out.println(rec.toString());
				println("[" + (i + 1) + "/" + targets.size() + "] " + vaHex + " " + label
						+ " -> " + (fn != null ? fn.getName() : "NO FUNCTION"));
			}
			out.println("  ]");
			out.println("}");
			out.close();
			println("wrote " + outFile);
		}
		finally {
			decomp.dispose();
		}

		// Full function inventory (address, name, size, signature) for search.
		if (funcFile != null) {
			PrintWriter fw = new PrintWriter(new OutputStreamWriter(
				new FileOutputStream(funcFile), StandardCharsets.UTF_8));
			fw.println("entry\tname\tsize\tsignature");
			FunctionIteratorHelper.dump(currentProgram, fw);
			fw.close();
			println("wrote " + funcFile);
		}

		// Defined strings.
		if (strFile != null) {
			PrintWriter sw = new PrintWriter(new OutputStreamWriter(
				new FileOutputStream(strFile), StandardCharsets.UTF_8));
			sw.println("address\tvalue");
			Listing listing = currentProgram.getListing();
			DataIteratorHelper.dumpStrings(listing, sw);
			sw.close();
			println("wrote " + strFile);
		}
	}

	private String nearestBefore(Address addr) {
		Function fn = currentProgram.getFunctionManager()
				.getFunctionContaining(addr);
		if (fn != null) {
			return fn.getEntryPoint().toString() + " " + fn.getName();
		}
		Function before = currentProgram.getFunctionManager()
				.getFunctionBefore(addr, true);
		return before == null ? "none"
				: before.getEntryPoint().toString() + " " + before.getName();
	}

	private String hexAt(Address addr, int n) {
		try {
			Memory mem = currentProgram.getMemory();
			byte[] b = new byte[n];
			int got = mem.getBytes(addr, b, 0, n);
			StringBuilder sb = new StringBuilder();
			for (int i = 0; i < got; i++) {
				if (i > 0) {
					sb.append(' ');
				}
				sb.append(String.format("%02x", b[i]));
			}
			return sb.toString();
		}
		catch (Exception e) {
			return "unreadable";
		}
	}

	/** Kept separate so the main script stays readable. */
	static class FunctionIteratorHelper {
		static void dump(ghidra.program.model.listing.Program program, PrintWriter fw) {
			ghidra.program.model.listing.FunctionIterator it =
				program.getFunctionManager().getFunctions(true);
			while (it.hasNext()) {
				Function f = it.next();
				fw.println(f.getEntryPoint() + "\t" + f.getName() + "\t" + f.getBody().getNumAddresses()
						+ "\t" + f.getSignature().toString().replace("\t", " ").replace("\n", " "));
			}
		}
	}

	static class DataIteratorHelper {
		static void dumpStrings(ghidra.program.model.listing.Listing listing, PrintWriter sw) {
			ghidra.program.model.listing.DataIterator it = listing.getDefinedData(true);
			while (it.hasNext()) {
				Data d = it.next();
				Object v = d.getValue();
				if (v == null) {
					continue;
				}
				String s = String.valueOf(v);
				if (s.isEmpty()) {
					continue;
				}
				sw.println(d.getAddress() + "\t" + s.replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t"));
			}
		}
	}
}

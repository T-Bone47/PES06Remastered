//FindImportCallers
//Finds every call site of a named import (or exported symbol) and decompiles
//the functions that contain them.
//Usage: -postScript FindImportCallers <name.txt> <out.txt>
//@category Analysis
//@author rea

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;

public class FindImportCallers extends GhidraScript {

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 2) {
			printerr("usage: FindImportCallers <names.txt> <out.txt>");
			return;
		}
		List<String> names = java.nio.file.Files.readAllLines(new File(args[0]).toPath());

		DecompInterface decomp = new DecompInterface();
		DecompileOptions opts = new DecompileOptions();
		decomp.setOptions(opts);
		decomp.openProgram(currentProgram);

		PrintWriter out = new PrintWriter(new OutputStreamWriter(
			new FileOutputStream(args[1]), StandardCharsets.UTF_8));

		for (String raw : names) {
			String nm = raw.trim();
			if (nm.isEmpty() || nm.startsWith("#")) {
				continue;
			}
			out.println();
			out.println("################ target: " + nm + " ################");
			Address targetAddr = null;
			if (nm.toLowerCase().startsWith("0x")) {
				long va = Long.parseLong(nm.substring(2), 16);
				targetAddr = currentProgram.getAddressFactory().getDefaultAddressSpace()
						.getAddress(va);
				out.println("  (resolving as address " + targetAddr + ")");
			}
			SymbolIterator sit = targetAddr == null
					? currentProgram.getSymbolTable().getSymbolIterator(nm, true)
					: null;
			int found = 0;
			Set<Function> owners = new LinkedHashSet<Function>();
			if (targetAddr != null) {
				ReferenceIterator trit = currentProgram.getReferenceManager()
						.getReferencesTo(targetAddr);
				while (trit.hasNext()) {
					Reference r = trit.next();
					Address from = r.getFromAddress();
					Function owner = currentProgram.getFunctionManager().getFunctionContaining(from);
					out.println("  " + from + "  " + r.getReferenceType() + " " + nm
							+ (owner == null ? "" : "   in " + owner.getName()));
					if (owner != null) {
						owners.add(owner);
					}
					found++;
				}
			}
			else {
			while (sit.hasNext()) {
				Symbol s = sit.next();
				if (s.getName() == null || !s.getName().equals(nm)) {
					continue;
				}
				Address a = s.getAddress();
				ReferenceIterator rit = currentProgram.getReferenceManager().getReferencesTo(a);
				while (rit.hasNext()) {
					Reference r = rit.next();
					Address from = r.getFromAddress();
					Function owner = currentProgram.getFunctionManager().getFunctionContaining(from);
					out.println("  " + from + "  " + r.getReferenceType() + " " + nm
							+ (owner == null ? "" : "   in " + owner.getName()));
					if (owner != null) {
						owners.add(owner);
					}
					found++;
				}
			}
			}
			out.println("  -> " + found + " call site(s), " + owners.size() + " function(s)");
			for (Function f : owners) {
				out.println();
				out.println("  // ===== " + f.getName() + " @ " + f.getEntryPoint() + " =====");
				try {
					DecompileResults res = decomp.decompileFunction(f, 120, monitor);
					if (res != null && res.decompileCompleted()) {
						out.println(res.getDecompiledFunction().getC());
					}
					else {
						out.println("  (decompile failed)");
					}
				}
				catch (Exception e) {
					out.println("  (exception: " + e.getMessage() + ")");
				}
			}
			out.flush();
			println(nm + ": " + found + " call sites");
		}
		out.close();
		decomp.dispose();
		println("wrote " + args[1]);
	}
}

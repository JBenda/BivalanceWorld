hi BsRight ctermfg=green ctermbg=None
hi BsWrong ctermfg=red ctermbg=None
hi BsError ctermfg=black ctermbg=red
hi BsUnknown ctermfg=white ctermbg=None
hi SignColumn ctermbg=None
sign define bs_wrong text=✗ texthl=BsWrong 
sign define bs_right text=✓ texthl=BsRight
sign define bs_error text=E texthl=BsError
sign define bs_unknown text=X texthl=BsUnknown

let s:chunks = ['']
let s:signs = {}
function! BivalenceSen#cb_stdout(job_id, data, event) dict
	let s:chunks[-1] .= a:data[0]
	call extend(s:chunks, a:data[1:])
	for i in range(len(s:chunks)-1)
		let l:args = split(s:chunks[i], ';')
		if s:signs[l:args[1]][l:args[2]] == -1
			let s:signs[l:args[1]][l:args[2]] = sign_place(0,
				\ 'Bivalence',
				\ l:args[0],
				\ l:args[1] + 0,
				\ {'lnum': l:args[2]})
		else
			call sign_place(s:signs[l:args[1]][l:args[2]],
						\ 'Bivalence',
						\ l:args[0],
						\ l:args[1] + 0)
		endif
	endfor
	let s:chunks = [s:chunks[-1]]
endfunction

function! BivalenceSen#cb_exit(job_id, data, event) dict
	for [key, value] in items(s:jobs)
		if value == a:job_id
			let s:jobs[key] = -1
		endif
	endfor
endfunction

let s:jobs = {}
function! BivalenceSen#init(bufnr)
	echom "init"
	let s:signs[a:bufnr] = {}
	let s:jobs[a:bufnr] = jobstart(['tkw_sen', fnamemodify(bufname(a:bufnr), ':r').'.wld'], {
				\ 'on_stdout': function('BivalenceSen#cb_stdout'),
				\ 'on_exit': function('BivalenceSen#cb_exit') })
endfunction

function! BivalenceSen#stop(bufnr)
	call jobstop(s:jobs[a:bufnr])
	let s:jobs[a:bufnr] = -1
endfunction

function! BivalenceSen#setSign(bufnr, ln, line)
	let l:ln = a:ln + 1
	call chansend(s:jobs[a:bufnr], "[a;".a:bufnr.";".l:ln.";".a:line."]\n")
	let s:signs[a:bufnr][l:ln] = -1
endfunction

function! BivalenceSen#updateSigns(bufnr)
	if ! has_key(s:jobs, a:bufnr) || s:jobs[a:bufnr] == -1
		call BivalenceSen#init(a:bufnr)
	endif
	call sign_unplace('Bivalence', {'buffer': a:bufnr})
	let s:signs[a:bufnr] = {}
	call chansend(s:jobs[a:bufnr], "[c]\n")
	let l:lines = getbufline(a:bufnr, '1', '$')
	let l:len = len(l:lines)
	call map(filter(range(l:len), 'l:lines[v:val] =~ "^ *\".*\",* *$"'), 'BivalenceSen#setSign(a:bufnr, v:val, l:lines[v:val])')
endfunction

au! BufRead,BufNewFile *.sen call BivalenceSen#init(bufnr())
au! BufWinEnter *.sen call BivalenceSen#updateSigns(bufnr())
au! InsertLeave,TextChanged *.sen call BivalenceSen#updateSigns(bufnr())

function LoadWorld()
	let l:file = bufname()
	let l:sen = fnameescape(fnamemodify(l:file, ':r').'.sen')
	if winnr('$') == 1
		let l:w = winwidth(0)
		exec 'vnew' . l:sen
		wincmd L
		vnew
		wincmd L
		1 wincmd w
		exec 'vert resize'.l:w/4
		2 wincmd w
		exec 'vert resize'.l:w/4
		3 wincmd w
		exec 'vert resize '.l:w/2
		let s:job =  termopen('tkw ' . l:file)
		1 wincmd w
	else
		call chansend(s:job, 'q')
		call jobstop(s:job)
		3 wincmd w
		new
		4 wincmd w
		bdelete!
		let s:job = termopen('tkw '. l:file)
		2 wincmd w
		exec 'e ' . l:sen
		1 wincmd w
	endif
endfunction

" au BufWinEnter *. call LoadWorld()
